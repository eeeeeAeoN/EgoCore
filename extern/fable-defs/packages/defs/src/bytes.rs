use bytemuck::{AnyBitPattern, NoUninit, PodCastError};
use derive_more::{Display, Error, From};
use std::{mem, str::Utf8Error, string::FromUtf16Error};

#[derive(Copy, Clone, Debug, PartialEq, Eq, Display, Error)]
#[display("Unexpected end of input")]
pub struct UnexpectedEnd;

/// Take bytes from the front of a slice.
pub fn take_bytes<'a>(bytes: &mut &'a [u8], size: usize) -> Result<&'a [u8], UnexpectedEnd> {
    if size > bytes.len() {
        Err(UnexpectedEnd)?
    }
    let (front, back) = bytes.split_at(size);
    *bytes = back;
    Ok(front)
}

#[derive(Copy, Clone, Debug, PartialEq, Eq, From, Display, Error)]
pub enum TakeError {
    UnexpectedEnd(UnexpectedEnd),
    PodCast(PodCastError),
}

/// Take a value that implements `bytemuck::AnyBitPattern` from the front of a byte slice.
pub fn take<T: AnyBitPattern>(bytes: &mut &[u8]) -> Result<T, TakeError> {
    let size = mem::size_of::<T>();
    let front = take_bytes(bytes, size)?;
    Ok(bytemuck::try_pod_read_unaligned(front)?)
}

/// Like `take` but ensures little endian
pub fn take_le<T: AnyBitPattern + Le>(bytes: &mut &[u8]) -> Result<T, TakeError> {
    take(bytes).map(Le::le)
}

/// Put bytes in front of a byte slice and advance forward.
pub fn put_bytes(out: &mut &mut [u8], inp: &[u8]) -> Result<(), UnexpectedEnd> {
    let index = inp.len();
    if index > out.len() {
        Err(UnexpectedEnd)?
    }
    let (front, back) = mem::take(out).split_at_mut(index);
    *out = back;
    front.copy_from_slice(inp);
    Ok(())
}

/// Put a value implementing `bytemuck::NoUninit` in front of a byte slice and advance forward.
pub fn put<T: NoUninit>(out: &mut &mut [u8], value: &T) -> Result<(), UnexpectedEnd> {
    put_bytes(out, bytemuck::bytes_of(value))
}

/// Like `put` but ensures little endian
pub fn put_le<T: NoUninit + Le>(out: &mut &mut [u8], value: &T) -> Result<(), UnexpectedEnd> {
    put(out, &value.le())
}

/// Take bytes up until a NUL byte.
pub fn take_null_terminated_bytes<'a>(bytes: &mut &'a [u8]) -> Result<&'a [u8], UnexpectedEnd> {
    let size = bytes.iter().position(|&x| x == 0).ok_or(UnexpectedEnd)?;
    let contents = take_bytes(bytes, size + 1)?;
    Ok(&contents[..contents.len() - 1])
}

/// Put bytes with a NUL byte.
pub fn put_null_terminated_bytes(out: &mut &mut [u8], value: &[u8]) -> Result<(), UnexpectedEnd> {
    put_bytes(out, value)?;
    put(out, &0u8)?;
    Ok(())
}

#[derive(Debug)]
pub enum TakeNullTerminatedUtf8 {
    Bytes(UnexpectedEnd),
    Utf8(Utf8Error),
}

/// Take UTF8 up until a NUL byte.
pub fn take_null_terminated_utf8<'a>(
    bytes: &mut &'a [u8],
) -> Result<&'a str, TakeNullTerminatedUtf8> {
    use TakeNullTerminatedUtf8 as E;
    let string_bytes = take_null_terminated_bytes(bytes).map_err(E::Bytes)?;
    let string = str::from_utf8(string_bytes).map_err(E::Utf8)?;
    Ok(string)
}

/// Put UTF8 bytes with a NUL byte.
pub fn put_null_terminated_utf8(out: &mut &mut [u8], value: &str) -> Result<(), UnexpectedEnd> {
    put_null_terminated_bytes(out, value.as_bytes())
}

#[derive(Debug)]
pub enum TakeNullTerminatedUtf16 {
    NullTerminatorPair,
    Bytes(UnexpectedEnd),
    FromUtf16(FromUtf16Error),
}

/// Take UTF16 string up until a pair of NUL bytes.
pub fn take_null_terminated_utf16(bytes: &mut &[u8]) -> Result<String, TakeNullTerminatedUtf16> {
    use TakeNullTerminatedUtf16 as E;

    let (chunks, _remainder) = bytes.as_chunks::<2>();

    let size = chunks
        .iter()
        .position(|&x| x == [0, 0])
        .ok_or(E::NullTerminatorPair)?;

    let byte_len = size * 2;
    let contents = take_bytes(bytes, byte_len + 2).map_err(E::Bytes)?;
    let contents = contents[..byte_len]
        .chunks_exact(2)
        .map(|chunk| u16::from_le_bytes([chunk[0], chunk[1]]))
        .collect::<Vec<_>>();
    let contents = String::from_utf16(&contents).map_err(E::FromUtf16)?;

    Ok(contents)
}

/// Put UTF16 bytes with a pair of NUL bytes.
pub fn put_null_terminated_utf16(out: &mut &mut [u8], value: &str) -> Result<(), UnexpectedEnd> {
    for unit in value.encode_utf16() {
        put(out, &unit.to_le())?;
    }
    put(out, &0u16)?;
    Ok(())
}

pub trait Le: Sized {
    fn le(self) -> Self;
}

impl Le for u8 {
    fn le(self) -> Self {
        self
    }
}

impl Le for u16 {
    fn le(self) -> Self {
        self.to_le()
    }
}

impl Le for i16 {
    fn le(self) -> Self {
        self.to_le()
    }
}

impl Le for i8 {
    fn le(self) -> Self {
        self
    }
}

impl Le for u64 {
    fn le(self) -> Self {
        self.to_le()
    }
}

impl Le for i32 {
    fn le(self) -> Self {
        self.to_le()
    }
}

impl Le for u32 {
    fn le(self) -> Self {
        self.to_le()
    }
}

impl Le for f32 {
    fn le(self) -> Self {
        f32::from_bits(self.to_bits().to_le())
    }
}
