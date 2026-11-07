//! Generic field walking.
//!
//! `#[derive(DefStruct)]` emits a `visit_fields` method that hands
//! each field to a [`FieldVisitor`] as a typed [`FieldRef`]. This lets a
//! consumer in another crate (the def compiler) drive uniform per-field logic —
//! e.g. applying text-def overrides — without defs depending on it, and
//! without orphan-rule trouble: defs produces the `FieldRef`, the
//! consumer only reads it.
//!
//! Containers are exposed through slot traits ([`VecSlot`], [`MapSlot`],
//! [`StructSlot`], [`VariantSlot`]) so the consumer can lower text-def
//! statements element-by-element without knowing the concrete types.

use crate::wire::{DefIndex, DefString, PString, VecMap, WStr};

/// Default construction for def wire types. Parallel to [`Default`] because
/// big fixed arrays (`[u8; 180]`, …) can't implement `Default` (orphan rule);
/// container slots and macro-generated struct defaults go through this trait.
pub trait DefDefault: Sized {
    fn def_default() -> Self;
}

macro_rules! def_default_impl {
    ($($ty:ty),+ $(,)?) => {
        $(
            impl DefDefault for $ty {
                fn def_default() -> Self {
                    Default::default()
                }
            }
        )+
    };
}

def_default_impl!(
    f32, i32, u32, bool, u8, u16, u64, i8, i16, String, WStr, DefIndex, PString
);

/// A `CDefString` defaults to -1 (the "no string" name-table offset) — the
/// game's `CDefString` constructor default, verified against retail (e.g. an
/// unset `OpinionDeedReactionDef::Animation` is -1, not 0).
impl DefDefault for DefString {
    fn def_default() -> Self {
        DefString(-1)
    }
}

impl<T: DefDefault> DefDefault for Vec<T> {
    fn def_default() -> Self {
        Vec::new()
    }
}

impl<K: DefDefault + Ord, V: DefDefault> DefDefault for std::collections::BTreeMap<K, V> {
    fn def_default() -> Self {
        std::collections::BTreeMap::new()
    }
}

impl<K: DefDefault, V: DefDefault> DefDefault for VecMap<K, V> {
    fn def_default() -> Self {
        VecMap(Vec::new())
    }
}

impl<T: DefDefault, const N: usize> DefDefault for [T; N] {
    fn def_default() -> Self {
        std::array::from_fn(|_| T::def_default())
    }
}

/// A closed def enum viewed as its wire `i32`, for generic field access.
pub trait EnumSlot {
    fn get_i32(&self) -> i32;
    /// Set from a wire/text value; `Err` carries an out-of-table value.
    fn set_i32(&mut self, value: i32) -> Result<(), i32>;
    fn symbol(&self) -> &'static str;
}

/// A def flags value viewed as its wire `i32` (total — any value is valid).
pub trait FlagsSlot {
    fn get_i32(&self) -> i32;
    fn set_i32(&mut self, value: i32);
    fn symbol(&self) -> Option<&'static str>;
    fn decompose_symbols(&self) -> Option<Vec<&'static str>>;
}

/// A typed, mutable handle to one def field.
pub enum FieldRef<'a> {
    F32(&'a mut f32),
    I32(&'a mut i32),
    U32(&'a mut u32),
    Bool(&'a mut bool),
    /// `CCharString` (UTF-8).
    Str(&'a mut String),
    /// `CWideString` (UTF-16).
    WStr(&'a mut WStr),
    Enum(&'a mut dyn EnumSlot),
    Flags(&'a mut dyn FlagsSlot),
    DefString(&'a mut DefString),
    DefIndex(&'a mut DefIndex),
    U8(&'a mut u8),
    U16(&'a mut u16),
    U64(&'a mut u64),
    I8(&'a mut i8),
    I16(&'a mut i16),
    /// Length-prefixed byte string (`u32` length + bytes).
    PString(&'a mut PString),
    /// `u32` count + elements.
    Vec(&'a mut dyn VecSlot),
    /// `u32` count + (key, value) pairs.
    Map(&'a mut dyn MapSlot),
    /// Fixed sequence of named member values (a `wire_struct!` compound).
    Struct(&'a mut dyn StructSlot),
    /// Tagged union (a `def_variant!` type).
    Variant(&'a mut dyn VariantSlot),
    /// Fixed-length array `[T; N]` (no count on the wire). Read-only element
    /// access so the differ/probe can descend into packed blobs (THING
    /// component data, morph/opinion arrays) that were previously opaque.
    Array(&'a mut dyn ArraySlot),
    /// A field the generic walk doesn't cover. Carries the field's Rust type
    /// name for diagnostics; the consumer handles these explicitly.
    Complex(&'static str),
}

/// Expose `&mut self` as a [`FieldRef`]. Implemented for the scalar/string
/// wire types here, for enum/flags types by the enum macros, and for the
/// container types below.
pub trait AsField {
    fn as_field(&mut self) -> FieldRef<'_>;
}

/// Receives each field of a def during `visit_fields` (see `#[derive(DefStruct)]`).
pub trait FieldVisitor {
    fn field(&mut self, name: &'static str, field: FieldRef<'_>);
}

/// Types whose fields can be visited generically (all `def_struct!` types).
pub trait VisitFields {
    fn visit_fields<V: FieldVisitor>(&mut self, visitor: &mut V);
}

/// Lets a `&mut dyn FieldVisitor` be passed where a generic `V: FieldVisitor`
/// is expected (so an object-safe visitor can drive [`VisitFields`]).
impl FieldVisitor for &mut (dyn FieldVisitor + '_) {
    fn field(&mut self, name: &'static str, field: FieldRef<'_>) {
        (**self).field(name, field)
    }
}

/// Element-wise mutable access to a `Vec` field.
pub trait VecSlot {
    fn len(&self) -> usize;
    fn is_empty(&self) -> bool {
        self.len() == 0
    }
    fn clear(&mut self);
    fn push_default(&mut self);
    fn element<'b>(&'b mut self, index: usize) -> FieldRef<'b>;
}

impl<T: AsField + DefDefault + Clone> VecSlot for Vec<T> {
    fn len(&self) -> usize {
        Vec::len(self)
    }
    fn clear(&mut self) {
        Vec::clear(self);
    }
    fn push_default(&mut self) {
        self.push(T::def_default());
    }
    fn element<'b>(&'b mut self, index: usize) -> FieldRef<'b> {
        self[index].as_field()
    }
}

/// A (key, value) pair under construction for a map field: the pair is built
/// outside the map (keys can't be mutated in place) and inserted on
/// [`commit`](MapEntrySlot::commit).
pub trait MapEntrySlot<'a> {
    fn key(&mut self) -> FieldRef<'_>;
    fn value(&mut self) -> FieldRef<'_>;
    fn commit(self: Box<Self>);
}

/// Pair-wise mutable access to a map field (`BTreeMap` or `VecMap`).
pub trait MapSlot {
    fn len(&self) -> usize;
    fn is_empty(&self) -> bool {
        self.len() == 0
    }
    fn clear(&mut self);
    /// Begin a new pair with default key and value.
    fn new_entry<'a>(&'a mut self) -> Box<dyn MapEntrySlot<'a> + 'a>;
    /// Read each existing (key, value) pair, in the map's stored order. Because
    /// a `BTreeMap`'s keys aren't mutably accessible (and [`FieldRef`] needs
    /// `&mut`), both key and value are exposed via short-lived owned clones —
    /// the callback must consume them within the call (the semantic decoder
    /// turns them into owned `SemVal`s immediately).
    fn for_each_pair(&self, f: &mut dyn FnMut(FieldRef<'_>, FieldRef<'_>));
}

struct BTreeMapEntry<'a, K, V> {
    map: &'a mut std::collections::BTreeMap<K, V>,
    key: K,
    value: V,
}

impl<'a, K: AsField + Ord + Clone, V: AsField + Clone> MapEntrySlot<'a>
    for BTreeMapEntry<'a, K, V>
{
    fn key(&mut self) -> FieldRef<'_> {
        self.key.as_field()
    }
    fn value(&mut self) -> FieldRef<'_> {
        self.value.as_field()
    }
    fn commit(self: Box<Self>) {
        let this = *self;
        this.map.insert(this.key, this.value);
    }
}

impl<K: AsField + Ord + DefDefault + Clone, V: AsField + DefDefault + Clone> MapSlot
    for std::collections::BTreeMap<K, V>
{
    fn len(&self) -> usize {
        self.len()
    }
    fn clear(&mut self) {
        self.clear();
    }
    fn new_entry<'a>(&'a mut self) -> Box<dyn MapEntrySlot<'a> + 'a> {
        Box::new(BTreeMapEntry {
            map: self,
            key: K::def_default(),
            value: V::def_default(),
        })
    }
    fn for_each_pair(&self, f: &mut dyn FnMut(FieldRef<'_>, FieldRef<'_>)) {
        for (k, v) in self.iter() {
            let mut k = k.clone();
            let mut v = v.clone();
            f(k.as_field(), v.as_field());
        }
    }
}

struct VecMapEntry<'a, K, V> {
    map: &'a mut VecMap<K, V>,
    key: K,
    value: V,
}

impl<'a, K: AsField + PartialEq + Clone, V: AsField + Clone> MapEntrySlot<'a>
    for VecMapEntry<'a, K, V>
{
    fn key(&mut self) -> FieldRef<'_> {
        self.key.as_field()
    }
    fn value(&mut self) -> FieldRef<'_> {
        self.value.as_field()
    }
    fn commit(self: Box<Self>) {
        let this = *self;
        this.map.insert(this.key, this.value);
    }
}

impl<K: AsField + PartialEq + DefDefault + Clone, V: AsField + DefDefault + Clone> MapSlot
    for VecMap<K, V>
{
    fn len(&self) -> usize {
        self.0.len()
    }
    fn clear(&mut self) {
        self.0.clear();
    }
    fn new_entry<'a>(&'a mut self) -> Box<dyn MapEntrySlot<'a> + 'a> {
        Box::new(VecMapEntry {
            map: self,
            key: K::def_default(),
            value: V::def_default(),
        })
    }
    fn for_each_pair(&self, f: &mut dyn FnMut(FieldRef<'_>, FieldRef<'_>)) {
        for (k, v) in self.0.iter() {
            let mut k = k.clone();
            let mut v = v.clone();
            f(k.as_field(), v.as_field());
        }
    }
}

/// Normalize a member name for matching: lowercase, underscores stripped.
/// Text-def member spellings follow the C++ members (`BankIndex`), the Rust
/// fields follow snake_case (`bank_index`); normalization unifies them.
pub fn normalize_member_name(name: &str) -> String {
    name.chars()
        .filter(|c| *c != '_')
        .flat_map(|c| c.to_lowercase())
        .collect()
}

/// Member-wise mutable access to a `wire_struct!` compound.
pub trait StructSlot {
    /// The compound's Rust type name (for diagnostics).
    fn type_name(&self) -> &'static str;
    fn member_count(&self) -> usize;
    /// Member Rust field name by declaration index.
    fn member_name(&self, index: usize) -> Option<&'static str>;
    fn member<'b>(&'b mut self, index: usize) -> Option<FieldRef<'b>>;
    /// Visit this compound's fields *by their def-script names* via
    /// [`VisitFields`], returning `true` if it did. `def_struct!` types (named
    /// fields, e.g. a def_struct used as a `Vec` element) override this to
    /// enable name-based nested lowering; positional `wire_struct!` compounds
    /// keep the default `false` and are lowered member-by-member instead.
    fn visit_named(&mut self, _visitor: &mut dyn FieldVisitor) -> bool {
        false
    }
}

/// Mutable access to a `def_variant!` tagged union.
pub trait VariantSlot {
    /// The variant enum's Rust type name (for ctor-name mapping tables).
    fn type_name(&self) -> &'static str;
    fn tag(&self) -> u32;
    /// Reset to the variant for `tag` with default field values. `false` when
    /// the tag isn't a known case.
    fn set_tag(&mut self, tag: u32) -> bool;
    /// The current variant's member count.
    fn member_count(&self) -> usize;
    fn member_name(&self, index: usize) -> Option<&'static str>;
    fn member<'b>(&'b mut self, index: usize) -> Option<FieldRef<'b>>;
}

impl AsField for f32 {
    fn as_field(&mut self) -> FieldRef<'_> {
        FieldRef::F32(self)
    }
}
impl AsField for i32 {
    fn as_field(&mut self) -> FieldRef<'_> {
        FieldRef::I32(self)
    }
}
impl AsField for u32 {
    fn as_field(&mut self) -> FieldRef<'_> {
        FieldRef::U32(self)
    }
}
impl AsField for bool {
    fn as_field(&mut self) -> FieldRef<'_> {
        FieldRef::Bool(self)
    }
}
impl AsField for String {
    fn as_field(&mut self) -> FieldRef<'_> {
        FieldRef::Str(self)
    }
}
impl AsField for WStr {
    fn as_field(&mut self) -> FieldRef<'_> {
        FieldRef::WStr(self)
    }
}
impl AsField for DefString {
    fn as_field(&mut self) -> FieldRef<'_> {
        FieldRef::DefString(self)
    }
}
impl AsField for DefIndex {
    fn as_field(&mut self) -> FieldRef<'_> {
        FieldRef::DefIndex(self)
    }
}
impl AsField for PString {
    fn as_field(&mut self) -> FieldRef<'_> {
        FieldRef::PString(self)
    }
}
impl AsField for u8 {
    fn as_field(&mut self) -> FieldRef<'_> {
        FieldRef::U8(self)
    }
}
impl AsField for u16 {
    fn as_field(&mut self) -> FieldRef<'_> {
        FieldRef::U16(self)
    }
}
impl AsField for u64 {
    fn as_field(&mut self) -> FieldRef<'_> {
        FieldRef::U64(self)
    }
}
impl AsField for i8 {
    fn as_field(&mut self) -> FieldRef<'_> {
        FieldRef::I8(self)
    }
}
impl AsField for i16 {
    fn as_field(&mut self) -> FieldRef<'_> {
        FieldRef::I16(self)
    }
}

impl<T: AsField + DefDefault + Clone> AsField for Vec<T> {
    fn as_field(&mut self) -> FieldRef<'_> {
        FieldRef::Vec(self)
    }
}
impl<K: AsField + Ord + DefDefault + Clone, V: AsField + DefDefault + Clone> AsField
    for std::collections::BTreeMap<K, V>
{
    fn as_field(&mut self) -> FieldRef<'_> {
        FieldRef::Map(self)
    }
}
impl<K: AsField + PartialEq + DefDefault + Clone, V: AsField + DefDefault + Clone> AsField
    for VecMap<K, V>
{
    fn as_field(&mut self) -> FieldRef<'_> {
        FieldRef::Map(self)
    }
}

/// Read-only element access to a fixed-length array `[T; N]`. Unlike [`VecSlot`]
/// it can't grow/clear (the length is fixed on the wire) — it exists so the
/// differ and probe can descend into packed arrays instead of treating them as
/// opaque. The generic lowering walk never populates arrays through this (they
/// are filled by bespoke ctor code), so no mutation entry points are needed.
pub trait ArraySlot {
    fn len(&self) -> usize;
    fn is_empty(&self) -> bool {
        self.len() == 0
    }
    fn element<'b>(&'b mut self, index: usize) -> FieldRef<'b>;
}

impl<T: AsField, const N: usize> ArraySlot for [T; N] {
    fn len(&self) -> usize {
        N
    }
    fn element<'b>(&'b mut self, index: usize) -> FieldRef<'b> {
        self[index].as_field()
    }
}

/// Fixed-size run of values. The generic *lowering* walk doesn't index into
/// arrays (they appear as positional ctor data in text, filled by bespoke
/// arms); but exposing them as [`FieldRef::Array`] lets the differ/probe see
/// their contents (previously an opaque blind spot — §9-R9).
impl<T: AsField, const N: usize> AsField for [T; N] {
    fn as_field(&mut self) -> FieldRef<'_> {
        FieldRef::Array(self)
    }
}
