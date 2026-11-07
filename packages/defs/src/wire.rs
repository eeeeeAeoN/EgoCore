//! The def wire model.
//!
//! A def body is a sequence of *controls*: `u32 crc32(field name)` followed by
//! the field's value. [`Wire`] describes how a VALUE (the bytes after the id)
//! parses, serializes, and measures — the field's Rust type fully determines
//! its wire format, so a def's parse/serialize/size can never disagree with
//! its struct declaration.
//!
//! Value encodings (verified byte-exactly against the retail `game.bin`,
//! `frontend.bin`, and `script.bin` — see `fable-decomp/defs-spec.json`):
//!
//! | Rust type            | wire                                            |
//! |----------------------|-------------------------------------------------|
//! | `f32`/`i32`/`u32`    | 4 bytes LE                                      |
//! | `bool`               | 1 byte (`0`/`1`; any non-zero parses as `true`) |
//! | `String`             | `CCharString`: UTF-8, NUL-terminated            |
//! | [`WStr`]             | `CWideString`: UTF-16LE, NUL(2)-terminated      |
//! | `Vec<T>`             | `u32` count, then `count × T`                   |
//! | `[T; N]`             | `N × T` (no count)                              |
//! | `BTreeMap<K, V>`     | `u32` count, then `count × (K, V)` in key order (C++ `std::map`) |
//! | [`VecMap<K, V>`]     | like `BTreeMap` but preserving stored order (C++ `CVectorMap`) |
//! | `DefEnum` types      | 4 bytes LE, must be in the C++ enum table       |
//! | `DefFlags` types     | 4 bytes LE, any value                           |
//! | `WireStruct` types   | member values in order (no inner ids)           |
//!
//! The typed def/compound/enum/flag types themselves are declared with the
//! `#[derive(DefStruct)]` / `#[derive(WireStruct)]` / `#[derive(DefVariant)]` /
//! `#[derive(DefEnum)]` / `#[derive(DefFlags)]` proc-macros (`defs-derive`).

use std::collections::BTreeMap;

use crate::binary::{
    ID_BYTE_SIZE, ParseControlError, ParseControlErrorReason, SerializeControlError,
    SerializeControlErrorReason,
};
use crate::bytes::{
    TakeError, TakeNullTerminatedUtf8, TakeNullTerminatedUtf16, UnexpectedEnd, put, put_le,
    put_null_terminated_utf8, put_null_terminated_utf16, take, take_le, take_null_terminated_utf8,
    take_null_terminated_utf16,
};
use crate::crc32::crc;

// ── Wire ──────────────────────────────────────────────────────────────────────

/// A value on the def wire: the bytes of one field after its control id (or of
/// one container element / compound member, which carry no ids of their own).
pub trait Wire: Sized {
    fn parse(cur: &mut &[u8]) -> Result<Self, ParseWireError>;

    fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd>;

    /// Exact number of bytes [`Wire::serialize`] writes.
    fn wire_size(&self) -> usize;
}

#[derive(Debug)]
pub enum ParseWireError {
    UnexpectedEnd,
    /// The value isn't in the field's C++ enum table.
    InvalidEnumValue {
        value: i32,
    },
    /// A tagged-union tag with no matching case.
    InvalidVariantTag(u32),
    Utf8(TakeNullTerminatedUtf8),
    Utf16(TakeNullTerminatedUtf16),
    /// Error inside the `index`th element of a container.
    Item {
        index: usize,
        inner: Box<ParseWireError>,
    },
    /// Error inside a named member of a `WireStruct` compound.
    Member {
        name: &'static str,
        inner: Box<ParseWireError>,
    },
    /// Error inside a sub-component: a value that is itself a stream of
    /// id-carrying field controls (e.g. one `UiDef` state).
    Field(Box<ParseControlError>),
}

impl From<ParseControlError> for ParseWireError {
    fn from(error: ParseControlError) -> Self {
        ParseWireError::Field(Box::new(error))
    }
}

impl SerializeControlError {
    /// All field-serialize failures are buffer exhaustion; unwrap to the plain
    /// [`UnexpectedEnd`] that [`Wire::serialize`] reports.
    pub fn unexpected_end(self) -> UnexpectedEnd {
        UnexpectedEnd
    }
}

impl From<TakeError> for ParseWireError {
    fn from(error: TakeError) -> Self {
        match error {
            // Def wire scalars are plain little-endian values; a `PodCast`
            // failure can only mean the input ran out mid-value.
            TakeError::UnexpectedEnd(UnexpectedEnd) | TakeError::PodCast(_) => {
                ParseWireError::UnexpectedEnd
            }
        }
    }
}

impl ParseWireError {
    pub fn item(index: usize) -> impl FnOnce(ParseWireError) -> ParseWireError {
        move |inner| ParseWireError::Item {
            index,
            inner: Box::new(inner),
        }
    }

    pub fn member(name: &'static str) -> impl FnOnce(ParseWireError) -> ParseWireError {
        move |inner| ParseWireError::Member {
            name,
            inner: Box::new(inner),
        }
    }
}

// ── scalars ───────────────────────────────────────────────────────────────────

macro_rules! wire_scalar {
    ($($ty:ty),+) => {
        $(
            impl Wire for $ty {
                fn parse(cur: &mut &[u8]) -> Result<Self, ParseWireError> {
                    Ok(take_le::<$ty>(cur)?)
                }

                fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd> {
                    put_le(out, self)
                }

                fn wire_size(&self) -> usize {
                    size_of::<$ty>()
                }
            }
        )+
    };
}

wire_scalar!(f32, i32, u32);

macro_rules! wire_scalar_int {
    ($($ty:ty),+) => {
        $(
            impl Wire for $ty {
                fn parse(cur: &mut &[u8]) -> Result<Self, ParseWireError> {
                    Ok(take_le::<$ty>(cur)?)
                }
                fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd> {
                    put_le(out, self)
                }
                fn wire_size(&self) -> usize {
                    size_of::<$ty>()
                }
            }
        )+
    };
}

wire_scalar_int!(u64, u16, u8, i16, i8);

/// A `CDefString`: a 4-byte reference into the compiled name table (its
/// `TablePos`). Distinct from a plain `i32` so def-string fields are visible in
/// the type; resolving the referenced string is a link-time concern.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Default)]
pub struct DefString(pub i32);

/// A `CDefIndex`: a 4-byte reference to another def's global entry index.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Default)]
pub struct DefIndex(pub i32);

macro_rules! wire_newtype_i32 {
    ($($ty:ident),+) => {
        $(
            impl Wire for $ty {
                fn parse(cur: &mut &[u8]) -> Result<Self, ParseWireError> {
                    Ok($ty(i32::parse(cur)?))
                }
                fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd> {
                    self.0.serialize(out)
                }
                fn wire_size(&self) -> usize {
                    size_of::<i32>()
                }
            }
        )+
    };
}

wire_newtype_i32!(DefString, DefIndex);

/// A length-prefixed byte string (`u32` length + raw bytes, no terminator), as
/// written by the game's `WriteString`/`ReadString`. Stored as bytes since it
/// isn't guaranteed valid UTF-8.
#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct PString(pub Vec<u8>);

impl Wire for PString {
    fn parse(cur: &mut &[u8]) -> Result<Self, ParseWireError> {
        let len = u32::parse(cur)? as usize;
        if cur.len() < len {
            return Err(ParseWireError::UnexpectedEnd);
        }
        let (bytes, rest) = cur.split_at(len);
        *cur = rest;
        Ok(PString(bytes.to_vec()))
    }

    fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd> {
        (self.0.len() as u32).serialize(out)?;
        crate::bytes::put_bytes(out, &self.0)
    }

    fn wire_size(&self) -> usize {
        size_of::<u32>() + self.0.len()
    }
}

impl Wire for bool {
    fn parse(cur: &mut &[u8]) -> Result<Self, ParseWireError> {
        Ok(take::<u8>(cur).map_err(ParseWireError::from)? != 0)
    }

    fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd> {
        put(out, &(*self as u8))
    }

    fn wire_size(&self) -> usize {
        size_of::<u8>()
    }
}

// ── strings ───────────────────────────────────────────────────────────────────

/// `CCharString`: UTF-8, NUL-terminated.
impl Wire for String {
    fn parse(cur: &mut &[u8]) -> Result<Self, ParseWireError> {
        take_null_terminated_utf8(cur)
            .map(str::to_owned)
            .map_err(ParseWireError::Utf8)
    }

    fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd> {
        put_null_terminated_utf8(out, self)
    }

    fn wire_size(&self) -> usize {
        self.len() + 1
    }
}

/// `CWideString`: UTF-16LE, NUL-terminated. A distinct type so the field's
/// declaration picks the encoding.
#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord, Hash, Default)]
pub struct WStr(pub String);

impl Wire for WStr {
    fn parse(cur: &mut &[u8]) -> Result<Self, ParseWireError> {
        take_null_terminated_utf16(cur)
            .map(WStr)
            .map_err(ParseWireError::Utf16)
    }

    fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd> {
        put_null_terminated_utf16(out, &self.0)
    }

    fn wire_size(&self) -> usize {
        (self.0.encode_utf16().count() + 1) * size_of::<u16>()
    }
}

impl std::fmt::Display for WStr {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        self.0.fmt(f)
    }
}

impl From<&str> for WStr {
    fn from(value: &str) -> Self {
        WStr(value.to_owned())
    }
}

impl From<String> for WStr {
    fn from(value: String) -> Self {
        WStr(value)
    }
}

// ── containers ────────────────────────────────────────────────────────────────

/// `u32` count, then the elements.
impl<T: Wire> Wire for Vec<T> {
    fn parse(cur: &mut &[u8]) -> Result<Self, ParseWireError> {
        let count = u32::parse(cur)?;
        (0..count as usize)
            .map(|i| T::parse(cur).map_err(ParseWireError::item(i)))
            .collect()
    }

    fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd> {
        (self.len() as u32).serialize(out)?;
        self.iter().try_for_each(|item| item.serialize(out))
    }

    fn wire_size(&self) -> usize {
        size_of::<u32>() + self.iter().map(Wire::wire_size).sum::<usize>()
    }
}

/// Fixed-size run of values, no count prefix.
impl<T: Wire, const N: usize> Wire for [T; N] {
    fn parse(cur: &mut &[u8]) -> Result<Self, ParseWireError> {
        let mut items = Vec::with_capacity(N);
        for i in 0..N {
            items.push(T::parse(cur).map_err(ParseWireError::item(i))?);
        }
        Ok(items.try_into().unwrap_or_else(|_| unreachable!()))
    }

    fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd> {
        self.iter().try_for_each(|item| item.serialize(out))
    }

    fn wire_size(&self) -> usize {
        self.iter().map(Wire::wire_size).sum()
    }
}

/// C++ `std::map`: `u32` count, then `(key, value)` pairs in key order.
impl<K: Wire + Ord, V: Wire> Wire for BTreeMap<K, V> {
    fn parse(cur: &mut &[u8]) -> Result<Self, ParseWireError> {
        let count = u32::parse(cur)?;
        (0..count as usize)
            .map(|i| {
                let key = K::parse(cur).map_err(ParseWireError::item(i))?;
                let value = V::parse(cur).map_err(ParseWireError::item(i))?;
                Ok((key, value))
            })
            .collect()
    }

    fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd> {
        (self.len() as u32).serialize(out)?;
        self.iter().try_for_each(|(key, value)| {
            key.serialize(out)?;
            value.serialize(out)
        })
    }

    fn wire_size(&self) -> usize {
        size_of::<u32>()
            + self
                .iter()
                .map(|(key, value)| key.wire_size() + value.wire_size())
                .sum::<usize>()
    }
}

/// A map that preserves stored pair order (C++ `CVectorMap` and friends).
/// Same wire shape as [`BTreeMap`], but round-trips whatever order the data
/// has instead of imposing key order.
#[derive(Debug, Clone, PartialEq, Default)]
pub struct VecMap<K, V>(pub Vec<(K, V)>);

impl<K: Wire, V: Wire> Wire for VecMap<K, V> {
    fn parse(cur: &mut &[u8]) -> Result<Self, ParseWireError> {
        let count = u32::parse(cur)?;
        (0..count as usize)
            .map(|i| {
                let key = K::parse(cur).map_err(ParseWireError::item(i))?;
                let value = V::parse(cur).map_err(ParseWireError::item(i))?;
                Ok((key, value))
            })
            .collect::<Result<_, _>>()
            .map(VecMap)
    }

    fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd> {
        (self.0.len() as u32).serialize(out)?;
        self.0.iter().try_for_each(|(key, value)| {
            key.serialize(out)?;
            value.serialize(out)
        })
    }

    fn wire_size(&self) -> usize {
        size_of::<u32>()
            + self
                .0
                .iter()
                .map(|(key, value)| key.wire_size() + value.wire_size())
                .sum::<usize>()
    }
}

impl<K: PartialEq, V> VecMap<K, V> {
    pub fn get(&self, key: &K) -> Option<&V> {
        self.0.iter().find(|(k, _)| k == key).map(|(_, v)| v)
    }

    /// Replace the value of an existing key or append a new pair.
    pub fn insert(&mut self, key: K, value: V) {
        match self.0.iter_mut().find(|(k, _)| *k == key) {
            Some((_, v)) => *v = value,
            None => self.0.push((key, value)),
        }
    }
}

// ── def classes ───────────────────────────────────────────────────────────

std::thread_local! {
    static LENIENT_CONTROL_IDS: std::cell::Cell<bool> = const { std::cell::Cell::new(false) };
}

/// Run `f` with control-ID validation relaxed (accepting any 4-byte control ID
/// and parsing values positionally, matching retail `game.exe`).
pub fn with_lenient_control_ids<R>(f: impl FnOnce() -> R) -> R {
    struct Guard(bool);
    impl Drop for Guard {
        fn drop(&mut self) {
            LENIENT_CONTROL_IDS.with(|c| c.set(self.0));
        }
    }
    let prev = LENIENT_CONTROL_IDS.with(|c| {
        let p = c.get();
        c.set(true);
        p
    });
    let _guard = Guard(prev);
    f()
}

/// Parse one field control: `u32 crc32(name)` (validated) then the value.
pub fn parse_field<T: Wire>(cur: &mut &[u8], name: &'static str) -> Result<T, ParseControlError> {
    let id = take_le::<u32>(cur).map_err(|inner| ParseControlError {
        name,
        reason: ParseControlErrorReason::MalformedId(inner),
    })?;

    let expected = crc(name.as_bytes());
    if id != expected && !LENIENT_CONTROL_IDS.with(|c| c.get()) {
        return Err(ParseControlError {
            name,
            reason: ParseControlErrorReason::WrongId {
                expected,
                found: id,
            },
        });
    }

    T::parse(cur).map_err(|inner| ParseControlError {
        name,
        reason: ParseControlErrorReason::Wire(inner),
    })
}

/// Serialize one field control: `u32 crc32(name)` then the value.
pub fn serialize_field<T: Wire>(
    out: &mut &mut [u8],
    name: &'static str,
    value: &T,
) -> Result<(), SerializeControlError> {
    put_le(out, &crc(name.as_bytes())).map_err(|inner| SerializeControlError {
        name,
        reason: SerializeControlErrorReason::MalformedId(inner),
    })?;

    value.serialize(out).map_err(|inner| SerializeControlError {
        name,
        reason: SerializeControlErrorReason::Value(inner),
    })
}

/// Size of one field control: id + value.
pub fn field_size<T: Wire>(value: &T) -> usize {
    ID_BYTE_SIZE + value.wire_size()
}

/// A wire value whose in-memory *shape* is chosen by a sibling field's value (a
/// "tag"), even though its wire encoding is a plain scalar. Used for polymorphic
/// fields like `UiDef.MeshIndex`, where the sibling `Type` decides whether the
/// stored `i32` is a graphics-bank asset id or a def reference. The derive routes
/// a field marked `#[def("Name", tag = "sibling")]` through [`parse_field_tagged`].
pub trait TaggedWire: Wire {
    /// Parse the value, using `tag` (the sibling field's already-parsed value) to
    /// pick the in-memory variant. Serialization stays plain [`Wire::serialize`].
    fn parse_tagged(cur: &mut &[u8], tag: i32) -> Result<Self, ParseWireError>;
}

/// Like [`parse_field`], but hands the sibling tag to [`TaggedWire::parse_tagged`]
/// so the field can pick its variant. The tag field must precede this one in
/// declaration order (the derive relies on that; see `DefStruct`).
pub fn parse_field_tagged<T: TaggedWire>(
    cur: &mut &[u8],
    name: &'static str,
    tag: i32,
) -> Result<T, ParseControlError> {
    let id = take_le::<u32>(cur).map_err(|inner| ParseControlError {
        name,
        reason: ParseControlErrorReason::MalformedId(inner),
    })?;
    let expected = crc(name.as_bytes());
    if id != expected && !LENIENT_CONTROL_IDS.with(|c| c.get()) {
        return Err(ParseControlError {
            name,
            reason: ParseControlErrorReason::WrongId {
                expected,
                found: id,
            },
        });
    }
    T::parse_tagged(cur, tag).map_err(|inner| ParseControlError {
        name,
        reason: ParseControlErrorReason::Wire(inner),
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::enums::{ControllerType, TableExpansion};

    fn round_trip<T: Wire + PartialEq + std::fmt::Debug>(value: T) {
        let mut buf = vec![0u8; value.wire_size()];
        let mut out = &mut buf[..];
        value.serialize(&mut out).unwrap();
        assert!(out.is_empty(), "wire_size over-estimated");
        let mut cur = &buf[..];
        let parsed = T::parse(&mut cur).unwrap();
        assert!(cur.is_empty(), "parse under-consumed");
        assert_eq!(parsed, value);
    }

    #[test]
    fn scalars_round_trip() {
        round_trip(1.5f32);
        round_trip(-7i32);
        round_trip(0xdead_beefu32);
        round_trip(true);
        round_trip(false);
    }

    #[test]
    fn strings_round_trip() {
        round_trip(String::from("ENG_ARIAL_24"));
        round_trip(String::new());
        round_trip(WStr::from("Data\\Video\\intro.wmv"));
        round_trip(WStr::default());
    }

    #[test]
    fn containers_round_trip() {
        round_trip(vec![1i32, 2, 3]);
        round_trip(Vec::<f32>::new());
        round_trip([0.5f32, -0.5]);
        round_trip(BTreeMap::from([
            (1u32, String::from("a")),
            (2, String::from("b")),
        ]));
        round_trip(VecMap(vec![
            (String::from("MINIMAP_ARENA"), 103i32),
            (String::from("MINIMAP_ABYSS"), 7),
        ]));
    }

    #[test]
    fn enums_round_trip() {
        round_trip(ControllerType::Mouse);
        round_trip(TableExpansion::HORIZONTAL | TableExpansion::VERTICAL);
    }

    #[test]
    fn enum_rejects_out_of_table() {
        let bytes = 99i32.to_le_bytes();
        let mut cur = &bytes[..];
        assert!(matches!(
            ControllerType::parse(&mut cur),
            Err(ParseWireError::InvalidEnumValue { value: 99 })
        ));
    }

    #[test]
    fn field_controls() {
        let mut buf = vec![0u8; field_size(&1.5f32)];
        let mut out = &mut buf[..];
        serialize_field(&mut out, "SeaHeight", &1.5f32).unwrap();
        let mut cur = &buf[..];
        let value: f32 = parse_field(&mut cur, "SeaHeight").unwrap();
        assert_eq!(value, 1.5);

        // wrong field name → WrongId
        let mut cur = &buf[..];
        assert!(parse_field::<f32>(&mut cur, "NotSeaHeight").is_err());
    }

    /// Test compound.
    #[derive(Debug, Clone, PartialEq, crate::WireStruct)]
    pub struct TestCompound {
        pub scale: f32,
        pub kind: ControllerType,
        pub names: Vec<String>,
    }

    #[test]
    fn wire_struct_round_trip_and_member_errors() {
        let value = TestCompound {
            scale: 2.0,
            kind: ControllerType::Keyboard,
            names: vec![String::from("a"), String::from("b")],
        };
        let mut buf = vec![0u8; value.wire_size()];
        let mut out = &mut buf[..];
        value.serialize(&mut out).unwrap();
        round_trip(value);

        // truncated input names the failing member
        let mut cur = &buf[..2];
        match TestCompound::parse(&mut cur) {
            Err(ParseWireError::Member { name: "scale", .. }) => {}
            other => panic!("expected member error on scale, got {other:?}"),
        }
    }

    // ── derive smoke tests (P0.5) ────────────────────────────────────────────

    #[derive(Debug, Clone, PartialEq, crate::DefStruct)]
    pub struct TestDefStructSmoke {
        #[def("Value")]
        pub value: i32,
    }

    #[test]
    fn def_struct_round_trip() {
        let value = TestDefStructSmoke { value: 42 };
        round_trip(value);
    }

    #[test]
    fn wire_struct_smoke_round_trip() {
        let value = TestCompound {
            scale: 1.0,
            kind: ControllerType::Keyboard,
            names: vec![],
        };
        round_trip(value);
    }

    #[derive(Debug, Clone, PartialEq, crate::DefVariant)]
    pub enum TestVariantSmoke {
        #[def(0)]
        Foo { x: i32 },
        #[def(1)]
        Bar { name: String },
    }

    #[test]
    fn def_variant_round_trip() {
        let v0 = TestVariantSmoke::Foo { x: 7 };
        round_trip(v0);
        let v1 = TestVariantSmoke::Bar {
            name: String::from("test"),
        };
        round_trip(v1);
    }

    #[derive(Debug, Clone, Copy, PartialEq, Eq, crate::DefEnum)]
    pub enum TestEnumSmoke {
        #[def("VALUE_A")]
        A = 0,
        #[def("VALUE_B")]
        B = 1,
    }

    #[test]
    fn def_enum_round_trip() {
        round_trip(TestEnumSmoke::A);
        round_trip(TestEnumSmoke::B);

        assert_eq!(TestEnumSmoke::A.symbol(), "VALUE_A");
        assert_eq!(
            TestEnumSmoke::from_symbol("VALUE_B"),
            Some(TestEnumSmoke::B)
        );
        assert_eq!(TestEnumSmoke::from_symbol("MISSING"), None);
    }

    #[test]
    fn def_enum_rejects_out_of_table() {
        let bytes = 99i32.to_le_bytes();
        let mut cur = &bytes[..];
        assert!(matches!(
            TestEnumSmoke::parse(&mut cur),
            Err(ParseWireError::InvalidEnumValue { value: 99 })
        ));
    }

    #[derive(Debug, Clone, Copy, PartialEq, Eq, crate::DefFlags)]
    #[flags(FLAG_ONE = 1 => "ONE", FLAG_TWO = 2 => "TWO")]
    pub struct TestFlagsSmoke(pub i32);

    #[test]
    fn def_flags_round_trip() {
        round_trip(TestFlagsSmoke(0));
        round_trip(TestFlagsSmoke(1));
        round_trip(TestFlagsSmoke(3));

        assert!(TestFlagsSmoke(3).contains(TestFlagsSmoke::FLAG_ONE));
        assert!(TestFlagsSmoke(3).contains(TestFlagsSmoke::FLAG_TWO));
        assert_eq!(TestFlagsSmoke::from_symbol("ONE"), Some(TestFlagsSmoke(1)));
        assert_eq!(TestFlagsSmoke::from_symbol("MISSING"), None);
    }

    #[test]
    fn enum_slot_symbol() {
        use crate::visit::EnumSlot;
        let a = TestEnumSmoke::A;
        let b = TestEnumSmoke::B;
        let slot_a: &dyn EnumSlot = &a;
        let slot_b: &dyn EnumSlot = &b;
        assert_eq!(slot_a.symbol(), "VALUE_A");
        assert_eq!(slot_b.symbol(), "VALUE_B");
    }

    #[test]
    fn flags_slot_decompose_symbols() {
        use crate::visit::FlagsSlot;

        // Single flag
        let f1 = TestFlagsSmoke::FLAG_ONE;
        let slot1: &dyn FlagsSlot = &f1;
        assert_eq!(f1.decompose_symbols(), Some(vec!["ONE"]));
        assert_eq!(slot1.symbol(), Some("ONE"));
        assert_eq!(slot1.decompose_symbols(), Some(vec!["ONE"]));

        // Combined flags
        let f_both = TestFlagsSmoke(3);
        let slot_both: &dyn FlagsSlot = &f_both;
        assert_eq!(f_both.decompose_symbols(), Some(vec!["ONE", "TWO"]));
        assert_eq!(slot_both.symbol(), None);
        assert_eq!(slot_both.decompose_symbols(), Some(vec!["ONE", "TWO"]));

        // Empty flags (0)
        let f_empty = TestFlagsSmoke(0);
        let slot_empty: &dyn FlagsSlot = &f_empty;
        assert_eq!(f_empty.decompose_symbols(), None);
        assert_eq!(slot_empty.symbol(), None);
        assert_eq!(slot_empty.decompose_symbols(), None);

        // Unknown bit
        let f_unk = TestFlagsSmoke(4);
        let slot_unk: &dyn FlagsSlot = &f_unk;
        assert_eq!(f_unk.decompose_symbols(), None);
        assert_eq!(slot_unk.decompose_symbols(), None);

        // Partial known + unknown bit
        let f_mixed = TestFlagsSmoke(5);
        let slot_mixed: &dyn FlagsSlot = &f_mixed;
        assert_eq!(f_mixed.decompose_symbols(), None);
        assert_eq!(slot_mixed.decompose_symbols(), None);
    }

    #[test]
    fn lenient_control_ids_test() {
        let mut buf = vec![0u8; field_size(&42i32)];
        let mut out = &mut buf[..];
        serialize_field(&mut out, "ActualField", &42i32).unwrap();

        // Under strict mode, wrong field ID fails
        let mut cur = &buf[..];
        assert!(matches!(
            parse_field::<i32>(&mut cur, "DifferentField"),
            Err(ParseControlError {
                reason: ParseControlErrorReason::WrongId { .. },
                ..
            })
        ));

        // Under lenient mode, wrong field ID is accepted and parses positionally
        let mut cur = &buf[..];
        let val = with_lenient_control_ids(|| {
            parse_field::<i32>(&mut cur, "DifferentField").unwrap()
        });
        assert_eq!(val, 42);
        assert!(cur.is_empty());

        // Strict mode is restored after with_lenient_control_ids returns
        let mut cur = &buf[..];
        assert!(parse_field::<i32>(&mut cur, "DifferentField").is_err());

        // Also test tagged field parsing under lenient mode
        #[derive(Debug, PartialEq)]
        struct DummyTagged(i32);
        impl Wire for DummyTagged {
            fn parse(cur: &mut &[u8]) -> Result<Self, ParseWireError> {
                Ok(DummyTagged(i32::parse(cur)?))
            }
            fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd> {
                self.0.serialize(out)
            }
            fn wire_size(&self) -> usize {
                self.0.wire_size()
            }
        }
        impl TaggedWire for DummyTagged {
            fn parse_tagged(cur: &mut &[u8], tag: i32) -> Result<Self, ParseWireError> {
                let val = i32::parse(cur)?;
                Ok(DummyTagged(val + tag))
            }
        }

        let mut cur = &buf[..];
        let tagged_val: DummyTagged = with_lenient_control_ids(|| {
            parse_field_tagged(&mut cur, "DifferentTaggedField", 10).unwrap()
        });
        assert_eq!(tagged_val, DummyTagged(52));
        assert!(cur.is_empty());

        let mut cur = &buf[..];
        assert!(parse_field_tagged::<DummyTagged>(&mut cur, "DifferentTaggedField", 10).is_err());
    }
}
