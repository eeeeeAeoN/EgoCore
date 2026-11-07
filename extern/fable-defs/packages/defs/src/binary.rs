use crate::{
    bytes::{TakeError, UnexpectedEnd, put, put_bytes, take, take_bytes},
    names::{Names, NamesEntry},
    wire::ParseWireError,
};
use std::{
    fs::File,
    io::{self, BufReader, Read},
    path::Path,
};

#[derive(Debug)]
pub struct DefBinary {
    pub header: DefBinaryHeader,
    pub name_refs: Vec<NameRef>,
    pub chunk_index: ChunkIndex,
    pub chunks: Vec<Chunk>,
}

/// One error type for the entire binary container parse layer, replacing the
/// former 14 per-function error enums. Each variant carries enough context to
/// trace the failure back to the specific field or index. Serialize failures
/// are always buffer exhaustion → [`UnexpectedEnd`].
#[derive(Debug)]
pub enum BinParseError {
    Io(io::Error),
    Decompress(miniz_oxide::inflate::DecompressError),
    /// A named field in a binary container struct (header, name-ref, chunk
    /// index entry, entry record, etc.).
    Field {
        name: &'static str,
        inner: Box<BinParseError>,
    },
    /// An indexed element in a list.
    Index {
        index: u32,
        inner: Box<BinParseError>,
    },
    /// Ran out of bytes mid-parse.
    UnexpectedEnd,
    /// A field-control parse failure (wrong CRC id, wire-level error).
    Control(ParseControlError),
    /// An entry offset table had a gap.
    InvalidOffset {
        start: usize,
        end: usize,
        expected: usize,
    },
    /// Entry name not found in the names table.
    NoNameEntry {
        position: u32,
    },
    /// Trailing bytes after all entries in a chunk.
    TrailingBytes {
        position: u32,
        remaining: usize,
        name: String,
    },
}

impl From<io::Error> for BinParseError {
    fn from(e: io::Error) -> Self {
        BinParseError::Io(e)
    }
}

impl From<TakeError> for BinParseError {
    fn from(e: TakeError) -> Self {
        match e {
            TakeError::UnexpectedEnd(_) | TakeError::PodCast(_) => BinParseError::UnexpectedEnd,
        }
    }
}

impl From<ParseControlError> for BinParseError {
    fn from(e: ParseControlError) -> Self {
        BinParseError::Control(e)
    }
}

impl DefBinary {
    pub fn load_with_names(path: &Path, names: &Names) -> Result<Self, BinParseError> {
        let file = File::open(path)?;
        let reader = BufReader::new(file);
        Self::from_reader_with_names(reader, names)
    }
}

impl DefBinary {
    pub fn from_reader_with_names<R: Read>(
        mut reader: R,
        names: &Names,
    ) -> Result<Self, BinParseError> {
        let mut buf = Vec::new();
        reader.read_to_end(&mut buf)?;
        Self::from_bytes_with_names(&buf, names)
    }
}

impl DefBinary {
    pub fn from_bytes_with_names(bytes: &[u8], names: &Names) -> Result<Self, BinParseError> {
        let bytes_cursor = &mut &bytes[..];

        let header = DefBinaryHeader::parse(bytes_cursor)?;

        let name_refs = NameRef::parse_list(bytes_cursor, header.entry_count)?;

        let chunk_index = ChunkIndex::parse(bytes_cursor)?;

        let chunks = Chunk::parse_list(bytes_cursor, &chunk_index, &name_refs, names)?;

        Ok(Self {
            header,
            name_refs,
            chunk_index,
            chunks,
        })
    }
}

#[derive(Debug)]
pub struct DefBinaryHeader {
    pub use_names_bin: bool,
    pub file_indicator: u32,
    pub platform_indicator: u32,
    pub entry_count: u32,
}

impl DefBinaryHeader {
    fn parse(cur: &mut &[u8]) -> Result<Self, BinParseError> {
        let use_names_bin = take::<u8>(cur).map_err(|e| BinParseError::Field {
            name: "use_names_bin",
            inner: Box::new(e.into()),
        })? == 0x1;
        let file_indicator = take::<u32>(cur)
            .map_err(|e| BinParseError::Field {
                name: "file_indicator",
                inner: Box::new(e.into()),
            })?
            .to_le();
        let platform_indicator = take::<u32>(cur)
            .map_err(|e| BinParseError::Field {
                name: "platform_indicator",
                inner: Box::new(e.into()),
            })?
            .to_le();
        let entry_count = take::<u32>(cur)
            .map_err(|e| BinParseError::Field {
                name: "entry_count",
                inner: Box::new(e.into()),
            })?
            .to_le();
        Ok(Self {
            use_names_bin,
            file_indicator,
            platform_indicator,
            entry_count,
        })
    }
}

impl DefBinaryHeader {
    pub fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd> {
        put(out, &(self.use_names_bin as u8))?;
        put(out, &self.file_indicator)?;
        put(out, &self.platform_indicator)?;
        put(out, &self.entry_count)?;
        Ok(())
    }
}

#[derive(Debug, Clone)]
pub struct NameRef {
    pub def_name_offset: u32,
    pub file_name_offset: u32,
    pub counter: u32,
}

impl NameRef {
    fn parse_list(cur: &mut &[u8], count: u32) -> Result<Vec<NameRef>, BinParseError> {
        (0..count)
            .map(|i| {
                Self::parse(cur).map_err(|e| BinParseError::Index {
                    index: i,
                    inner: Box::new(e),
                })
            })
            .collect()
    }
}

impl NameRef {
    fn parse(cur: &mut &[u8]) -> Result<Self, BinParseError> {
        let def_name_offset = take::<u32>(cur)
            .map_err(|e| BinParseError::Field {
                name: "def_name_offset",
                inner: Box::new(e.into()),
            })?
            .to_le();
        let file_name_offset = take::<u32>(cur)
            .map_err(|e| BinParseError::Field {
                name: "file_name_offset",
                inner: Box::new(e.into()),
            })?
            .to_le();
        let counter = take::<u32>(cur)
            .map_err(|e| BinParseError::Field {
                name: "counter",
                inner: Box::new(e.into()),
            })?
            .to_le();
        Ok(Self {
            def_name_offset,
            file_name_offset,
            counter,
        })
    }
}

impl NameRef {
    pub fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd> {
        put(out, &self.def_name_offset)?;
        put(out, &self.file_name_offset)?;
        put(out, &self.counter)?;
        Ok(())
    }
}

#[derive(Debug)]
pub struct ChunkIndex {
    pub header: ChunkIndexHeader,
    pub entries: Vec<ChunkIndexEntry>,
}

impl ChunkIndex {
    fn parse(cur: &mut &[u8]) -> Result<Self, BinParseError> {
        let header = ChunkIndexHeader::parse(cur)?;
        let mut entries = Vec::new();

        for _ in 0..(header.chunk_count - 1) {
            entries.push(ChunkIndexEntry::parse(cur)?);
        }

        let _sentinel = ChunkIndexEntry::parse_sentinel(cur)?;

        Ok(Self { header, entries })
    }
}

#[derive(Debug)]
pub struct ChunkIndexHeader {
    pub chunk_count: u32,
    pub reserved: u32,
}

impl ChunkIndexHeader {
    fn parse(cur: &mut &[u8]) -> Result<Self, BinParseError> {
        let chunk_count = take::<u32>(cur)
            .map_err(|e| BinParseError::Field {
                name: "chunk_count",
                inner: Box::new(e.into()),
            })?
            .to_le();
        let reserved = take::<u32>(cur)
            .map_err(|e| BinParseError::Field {
                name: "reserved",
                inner: Box::new(e.into()),
            })?
            .to_le();
        Ok(Self {
            chunk_count,
            reserved,
        })
    }
}

impl ChunkIndexHeader {
    pub fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd> {
        put(out, &self.chunk_count)?;
        put(out, &self.reserved)?;
        Ok(())
    }
}

#[derive(Debug)]
pub struct ChunkIndexEntry {
    pub compressed_offset: u32,
    pub cumulative_entry_count: u32,
}

impl ChunkIndexEntry {
    fn parse(cur: &mut &[u8]) -> Result<Self, BinParseError> {
        let compressed_offset = take::<u32>(cur)
            .map_err(|e| BinParseError::Field {
                name: "compressed_offset",
                inner: Box::new(e.into()),
            })?
            .to_le();
        let cumulative_entry_count = take::<u32>(cur)
            .map_err(|e| BinParseError::Field {
                name: "cumulative_entry_count",
                inner: Box::new(e.into()),
            })?
            .to_le();
        Ok(Self {
            compressed_offset,
            cumulative_entry_count,
        })
    }
}

impl ChunkIndexEntry {
    pub fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd> {
        put(out, &self.compressed_offset)?;
        put(out, &self.cumulative_entry_count)?;
        Ok(())
    }
}

impl ChunkIndexEntry {
    fn parse_sentinel(cur: &mut &[u8]) -> Result<Option<Self>, BinParseError> {
        let mut input_copied = *cur;

        let sentinel_entry = ChunkIndexEntry::parse(&mut input_copied)?;

        let matches = sentinel_entry.compressed_offset == sentinel_entry.cumulative_entry_count
            && sentinel_entry.compressed_offset == input_copied.len() as u32;

        if matches {
            *cur = input_copied;
        }

        Ok(if matches { Some(sentinel_entry) } else { None })
    }
}

impl Chunk {
    fn parse_list(
        cur: &mut &[u8],
        chunk_index: &ChunkIndex,
        name_refs: &[NameRef],
        names: &Names,
    ) -> Result<Vec<Self>, BinParseError> {
        let mut list = Vec::new();
        let mut chunk_entry_base = 0;

        for (i, entry) in chunk_index.entries.iter().enumerate() {
            let start = entry.compressed_offset;

            let end = chunk_index
                .entries
                .get(i + 1)
                .map(|x| x.compressed_offset)
                .unwrap_or(cur.len() as u32);

            let mut compressed_data = &cur[start as usize..end as usize];

            let compressed_data_cursor = &mut compressed_data;

            let chunk_entry_count = entry.cumulative_entry_count - chunk_entry_base;

            let chunk = Chunk::parse(
                compressed_data_cursor,
                chunk_entry_base,
                chunk_entry_count,
                name_refs,
                names,
            )?;

            list.push(chunk);

            chunk_entry_base = entry.cumulative_entry_count;
        }

        Ok(list)
    }
}

#[derive(Debug)]
pub struct Chunk {
    pub entry_base: u32,
    pub entry_count: u32,
    pub entries: Vec<EntryRecord>,
}

const MAX_CHUNK_DECOMPRESS_SIZE: usize = 32768; // 32KiB, just a guess for now

impl Chunk {
    fn parse(
        cur: &mut &[u8],
        entry_base: u32,
        entry_count: u32,
        name_refs: &[NameRef],
        names: &Names,
    ) -> Result<Self, BinParseError> {
        let decompressed_bytes =
            miniz_oxide::inflate::decompress_to_vec_zlib_with_limit(cur, MAX_CHUNK_DECOMPRESS_SIZE)
                .map_err(BinParseError::Decompress)?;

        let decompressed_bytes_cursor = &mut &decompressed_bytes[..];

        let entries = EntryRecord::parse_list(
            decompressed_bytes_cursor,
            entry_base,
            entry_count,
            name_refs,
            names,
        )?;

        if !decompressed_bytes_cursor.is_empty() {
            return Err(BinParseError::TrailingBytes {
                position: entry_base,
                remaining: decompressed_bytes_cursor.len(),
                name: String::new(),
            });
        }

        Ok(Self {
            entry_base,
            entry_count,
            entries,
        })
    }

    pub fn from_entries(entry_base: u32, entries: Vec<EntryRecord>) -> Self {
        let entry_count = entries.len() as u32;
        Self {
            entry_base,
            entry_count,
            entries,
        }
    }

    pub fn to_bytes(&self) -> Vec<u8> {
        let entry_count = self.entries.len();
        let offset_table_size = entry_count * size_of::<u16>();
        let mut payload_size = 0usize;
        for entry in &self.entries {
            payload_size += entry.byte_size();
        }
        let decompressed_size = offset_table_size + payload_size;
        let mut buf = vec![0u8; decompressed_size];
        let mut offset = offset_table_size as u16;
        for (i, entry) in self.entries.iter().enumerate() {
            buf[i * 2..i * 2 + 2].copy_from_slice(&offset.to_le_bytes());
            offset += entry.byte_size() as u16;
        }
        let mut cur: &mut [u8] = &mut buf[offset_table_size..];
        for entry in &self.entries {
            entry.serialize(&mut cur).unwrap();
        }
        // Retail def binaries compress every chunk at zlib level 1
        // (78 01 header); match them so our output mirrors the files the
        // game's loader was built against.
        miniz_oxide::deflate::compress_to_vec_zlib(&buf, 1)
    }
}

#[derive(Debug, Clone)]
pub struct EntryRecord {
    pub preamble: EntryPreamble,
    /// The def's sub-def table: `Some` (possibly empty) for def types deriving
    /// from the sub-def bases, `None` for all other types. Presence is a
    /// per-type property — see [`def_name_has_subdef_table`].
    pub sub_defs: Option<Vec<SubDefRecord>>,
    pub chunk_start: usize,
    pub chunk_end: usize,
    pub body: DefBody,
    pub raw_bytes: Vec<u8>,
}

/// One record of a def's sub-def table, written between the entry preamble
/// and the field controls for def types deriving from the sub-def bases:
/// `u16` count, then `count` of these 12-byte records. Each record links the
/// entry (`owner_index`, usually its own global index) to another def entry
/// (`def_index`, an unnamed compiler-generated sub-def), keyed by the
/// sub-def's `name_crc`. Verified against all three retail bins.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct SubDefRecord {
    pub name_crc: u32,
    pub def_index: u32,
    pub owner_index: u32,
}

impl SubDefRecord {
    fn parse(cur: &mut &[u8]) -> Result<Self, TakeError> {
        let name_crc = take::<u32>(cur)?.to_le();
        let def_index = take::<u32>(cur)?.to_le();
        let owner_index = take::<u32>(cur)?.to_le();
        Ok(Self {
            name_crc,
            def_index,
            owner_index,
        })
    }

    pub fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd> {
        put(out, &self.name_crc)?;
        put(out, &self.def_index)?;
        put(out, &self.owner_index)
    }

    pub const BYTE_SIZE: usize = 12;
}

impl EntryRecord {
    pub fn parse_list(
        cur: &mut &[u8],
        chunk_entry_base: u32,
        chunk_entry_count: u32,
        name_refs: &[NameRef],
        names: &Names,
    ) -> Result<Vec<Self>, BinParseError> {
        let original_cur = &mut &cur[..];

        let mut offsets = Vec::new();

        for _ in 0..chunk_entry_count {
            let offset = take::<u16>(cur)
                .map_err(|e| BinParseError::Field {
                    name: "offset",
                    inner: Box::new(e.into()),
                })?
                .to_le();
            offsets.push(offset)
        }

        let payload_base = chunk_entry_count as usize * 2;

        let mut expected_chunk_start = payload_base;

        let mut entries = Vec::new();

        for i in 0..chunk_entry_count {
            let chunk_start = offsets[i as usize] as usize;

            let chunk_end = offsets
                .get(i as usize + 1)
                .map(|&x| x as usize)
                .unwrap_or(original_cur.len());

            if chunk_start != expected_chunk_start {
                return Err(BinParseError::InvalidOffset {
                    start: chunk_start,
                    end: chunk_end,
                    expected: expected_chunk_start,
                });
            }

            expected_chunk_start = chunk_end;

            let entry_len = chunk_end - chunk_start;

            let entry_position = chunk_entry_base + i;
            let entry_name_ref = &name_refs[entry_position as usize];
            let entry_name = &names.map.get(&entry_name_ref.def_name_offset);

            match entry_name {
                Some(entry_name) => {
                    let mut entry_bytes =
                        take_bytes(cur, entry_len).map_err(|_| BinParseError::UnexpectedEnd)?;

                    let entry_bytes_cursor = &mut entry_bytes;

                    let entry_record =
                        EntryRecord::parse(entry_bytes_cursor, entry_name, chunk_start, chunk_end)
                            .map_err(|error| BinParseError::Index {
                                index: entry_position,
                                inner: Box::new(error),
                            })?;

                    if !entry_bytes_cursor.is_empty() {
                        return Err(BinParseError::TrailingBytes {
                            position: entry_position,
                            remaining: entry_bytes_cursor.len(),
                            name: entry_name.string.clone(),
                        });
                    }

                    entries.push(entry_record);
                }
                None => {
                    return Err(BinParseError::NoNameEntry {
                        position: entry_position,
                    });
                }
            }
        }

        Ok(entries)
    }
}

impl EntryRecord {
    fn parse(
        cur: &mut &[u8],
        name: &NamesEntry,
        chunk_start: usize,
        chunk_end: usize,
    ) -> Result<Self, BinParseError> {
        let raw_bytes = cur.to_vec();

        let preamble = EntryPreamble::parse(cur)?;

        let sub_defs = if def_name_has_subdef_table(&name.string) {
            let count = take::<u16>(cur)
                .map_err(|e| BinParseError::Field {
                    name: "sub_def_count",
                    inner: Box::new(e.into()),
                })?
                .to_le();
            let mut records = Vec::with_capacity(count as usize);
            for _ in 0..count {
                records.push(SubDefRecord::parse(cur).map_err(|e| BinParseError::Field {
                    name: "sub_def",
                    inner: Box::new(e.into()),
                })?);
            }
            Some(records)
        } else {
            None
        };

        let mut attempt = *cur;
        let body =
            DefBody::parse(&mut attempt, &name.string).map_err(|(_, e)| BinParseError::from(e))?;
        *cur = attempt;

        Ok(Self {
            chunk_start,
            chunk_end,
            raw_bytes,
            preamble,
            sub_defs,
            body,
        })
    }
}

impl EntryRecord {
    pub fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd> {
        self.preamble.serialize(out)?;
        if let Some(records) = &self.sub_defs {
            put(out, &(records.len() as u16))?;
            for record in records {
                record.serialize(out)?;
            }
        }
        self.body.serialize(out)?;
        Ok(())
    }

    pub fn byte_size(&self) -> usize {
        EntryPreamble::BYTE_SIZE
            + self
                .sub_defs
                .as_ref()
                .map_or(0, |r| 2 + r.len() * SubDefRecord::BYTE_SIZE)
            + self.body.byte_size()
    }

    pub fn payload_size(&self) -> usize {
        self.chunk_end - self.chunk_start
    }
}

// ── DefBody: generated from the canonical def table ──────────────────────────

macro_rules! def_body {
    ($($variant:ident ( $type:ident ) => [$($name:literal),+ $(,)?]),+ $(,)?) => {
        #[allow(clippy::large_enum_variant)]
        #[derive(Debug, Clone)]
        pub enum DefBody {
            $( $variant(crate::def::$type), )+
        }

        impl DefBody {
            pub(crate) fn parse<'a>(
                cur: &mut &[u8],
                name: &'a str,
            ) -> Result<Self, (&'a str, ParseControlError)> {
                Ok(match name {
                    $(
                        $( $name => DefBody::$variant(
                            crate::def::$type::parse(cur).map_err(|e| (name, e))?
                        ), )+
                    )+
                    _ => {
                        return Err((name, ParseControlError {
                            name: "<unknown type>",
                            reason: crate::binary::ParseControlErrorReason::MalformedId(
                                crate::bytes::TakeError::UnexpectedEnd(crate::bytes::UnexpectedEnd)),
                        }))
                    }
                })
            }

            pub fn serialize(
                &self,
                out: &mut &mut [u8],
            ) -> Result<(), UnexpectedEnd> {
                match self {
                    $( Self::$variant(d) => d.serialize(out).map_err(|e| e.unexpected_end()), )+
                }
            }

            pub fn byte_size(&self) -> usize {
                match self {
                    $( Self::$variant(d) => d.byte_size(), )+
                }
            }

            /// Visit the active variant's fields via reflection (drives the
            /// semantic differ / SemVal decoder (see `examples/verify.rs`).
            pub fn visit_active(&mut self, visitor: &mut dyn crate::visit::FieldVisitor) {
                use crate::visit::VisitFields as _;
                let mut visitor: &mut dyn crate::visit::FieldVisitor = visitor;
                match self {
                    $( Self::$variant(d) => crate::def::$type::visit_fields(d, &mut visitor), )+
                }
            }

            pub fn def_default_for_name(name: &str) -> Option<Self> {
                use crate::visit::DefDefault as _;
                Some(match name {
                    $(
                        $( $name => Self::$variant(<crate::def::$type>::def_default()), )+
                    )+
                    _ => return None,
                })
            }
        }

        impl crate::visit::VisitFields for DefBody {
            fn visit_fields<V: crate::visit::FieldVisitor>(&mut self, visitor: &mut V) {
                self.visit_active(visitor);
            }
        }
    };
}

def_body! {
    AbilityDef(AbilityDef) => ["CAbilityDef"],
    ActionUseDef(ActionUseDef) => ["CActionUseDef"],
    ActivateQuestDef(ActivateQuestDef) => ["CActivateQuestDef"],
    AICreatureWillPowerIndicatorDef(AICreatureWillPowerIndicatorDef) => ["CAICreatureWillPowerIndicatorDef"],
    AIScratchpadDef(AIScratchpadDef) => ["CAIScratchpadDef"],
    AnimatingObjectDef(AnimatingObjectDef) => ["CAnimatingObjectDef"],
    AppearanceDef(AppearanceDef) => ["CAppearanceDef"],
    AreaOfEffectAttackDef(AreaOfEffectAttackDef) => ["CAreaOfEffectAttackDef"],
    ArmourDef(ArmourDef) => ["ARMOUR"],
    AttackPatternDef(AttackPatternDef) => ["ATTACK_PATTERN"],
    AugmentationDef(AugmentationDef) => ["CAugmentationDef"],
    BalverineBattleDef(BalverineBattleDef) => ["CBalverineBattleDef"],
    BedDef(BedDef) => ["CBedDef"],
    BettingDef(BettingDef) => ["CBettingDef"],
    BoastingPodiumDef(BoastingPodiumDef) => ["CBoastingPodiumDef"],
    BonusItemDef(BonusItemDef) => ["CBonusItemDef"],
    BossDef(BossDef) => ["CBossDef"],
    BrainDef(BrainDef) => ["BRAIN"],
    BriarRoseDef(BriarRoseDef) => ["CBriarRoseDef"],
    BuyHouseDef(BuyHouseDef) => ["CBuyHouseDef"],
    BuyableHouseDef(BuyableHouseDef) => ["CBuyableHouseDef"],
    CameraCollisionDef(CameraCollisionDef) => ["CCameraCollisionDef"],
    CameraManagerDef(CameraManagerDef) => ["CAMERA_MANAGER"],
    CameraManagerSetDef(CameraManagerSetDef) => ["CAMERA_MANAGER_SET"],
    CameraModeDef(CameraModeDef) => ["CAMERA_MODE"],
    CarriedReadableDef(CarriedReadableDef) => ["CCarriedReadableDef"],
    CarrySlotDef(CarrySlotDef) => ["CARRY_SLOT"],
    CarryableDef(CarryableDef) => ["CCarryableDef"],
    CarryingDef(CarryingDef) => ["CCarryingDef"],
    ChestDef(ChestDef) => ["CChestDef"],
    ClockDef(ClockDef) => ["CClockDef"],
    CoinGameObstacleDef(CoinGameObstacleDef) => ["CCoinGameObstacleDef"],
    CombatAbilityAttackBase(CombatAbilityAttackBase) => ["CCombatAbilityBlockCounterAttackDef", "CCombatAbilityFlourishCounterAttackDef", "CCombatAbilityGetHitCounterAttackDef"],
    CombatAbilityBlockDefBase(CombatAbilityBlockDefBase) => ["CCombatAbilityBlockHeavyWeaponAttackDef", "CCombatAbilityBlockLightWeaponAttackDef", "CCombatAbilityBlockProjectileWeaponAttackDef", "CCombatAbilityBlockUnarmedAttackDef"],
    CombatAbilityStrafeDef(CombatAbilityStrafeDef) => ["CCombatAbilityStrafeDef"],
    CombatAbilityUseProjectileWeaponDef(CombatAbilityUseProjectileWeaponDef) => ["CCombatAbilityUseProjectileWeaponDef"],
    CombatDialogueDef(CombatDialogueDef) => ["COMBAT_DIALOGUE_DEF"],
    CombatSequenceDef(CombatSequenceDef) => ["COMBAT_SEQUENCE"],
    CombatTypeDef(CombatTypeDef) => ["COMBAT_TYPE"],
    ContainerRewardHeroDef(ContainerRewardHeroDef) => ["CContainerRewardHeroDef"],
    ContextSensitiveItemDef(ContextSensitiveItemDef) => ["CContextSensitiveItemDef"],
    CoopSpiritDef(CoopSpiritDef) => ["CCoopSpiritDef"],
    CrateStackDef(CrateStackDef) => ["CCrateStackDef"],
    CreatureAbilityDef(CreatureAbilityDef) => ["CREATURE_ABILITY"],
    CreatureDef(CreatureDef) => ["CCreatureDef"],
    CreatureGenerationFamilyDef(CreatureGenerationFamilyDef) => ["CREATURE_GENERATION_FAMILY"],
    CreatureGeneratorDef(CreatureGeneratorDef) => ["CCreatureGeneratorDef"],
    CreatureModeDef(CreatureModeDef) => ["CCreatureModeDef"],
    CreatureNavigationDef(CreatureNavigationDef) => ["CCreatureNavigationDef"],
    CreatureStatsDef(CreatureStatsDef) => ["CCreatureStatsDef"],
    CutsceneDef(CutsceneDef) => ["CCutsceneDef"],
    DecapitationDef(DecapitationDef) => ["CDecapitationDef"],
    DoorDef(DoorDef) => ["CDoorDef"],
    DragonActionHoverDef(DragonActionHoverDef) => ["CDragonActionHoverDef"],
    DragonActionNapalmDef(DragonActionNapalmDef) => ["CDragonActionNapalmDef"],
    DragonActionSwoopDef(DragonActionSwoopDef) => ["CDragonActionSwoopDef"],
    DrunkennessDef(DrunkennessDef) => ["CDrunkennessDef"],
    EnemyDef(EnemyDef) => ["CEnemyDef"],
    EngineLocalDetailGeneratorDef(EngineLocalDetailGeneratorDef) => ["LOCAL_DETAIL_GENERATOR"],
    EngineThemeDef(EngineThemeDef) => ["ENGINE_THEME"],
    EngineThemeGroupDef(EngineThemeGroupDef) => ["ENGINE_THEME_GROUP", "THING_GROUP"],
    EntitySoundDef(EntitySoundDef) => ["CEntitySoundDef"],
    ExperienceDef(ExperienceDef) => ["CExperienceDef"],
    ExplodingObjectDef(ExplodingObjectDef) => ["CExplodingObjectDef"],
    ExplosionDef(ExplosionDef) => ["CExplosionDef"],
    ExplosiveTrailDef(ExplosiveTrailDef) => ["CExplosiveTrailDef"],
    ExpressionDef(ExpressionDef) => ["EXPRESSION"],
    ExpressionSubDef(ExpressionSubDef) => ["CExpressionSubDef"],
    FactionDef(FactionDef) => ["FACTION"],
    FireballSpellLevelDef(FireballSpellLevelDef) => ["CFireballSpellLevelDef"],
    FireheartMinigameDef(FireheartMinigameDef) => ["CFireheartMinigameDef"],
    FishDef(FishDef) => ["CFishDef"],
    FishingDef(FishingDef) => ["CFishingDef"],
    FishingRodDef(FishingRodDef) => ["CFishingRodDef"],
    FlammableDef(FlammableDef) => ["CFlammableDef"],
    GiftDef(GiftDef) => ["CGiftDef"],
    GoldDef(GoldDef) => ["CGoldDef"],
    GuardDef(GuardDef) => ["CGuardDef"],
    GuildMasterDef(GuildMasterDef) => ["CGuildMasterDef"],
    HairCardDef(HairCardDef) => ["CHairCardDef"],
    HasNameDef(HasNameDef) => ["CHasNameDef"],
    HeroAbilityDef(HeroAbilityDef) => ["HERO_ABILITY"],
    HeroCentreDef(HeroCentreDef) => ["CHeroCentreDef"],
    HeroCombatDef(HeroCombatDef) => ["HERO_COMBAT"],
    HeroDef(HeroDef) => ["CHeroDef"],
    HeroExperienceDef(HeroExperienceDef) => ["CHeroExperienceDef"],
    HeroMarriageDef(HeroMarriageDef) => ["CHeroMarriageDef"],
    HeroMorphDef(HeroMorphDef) => ["CHeroMorphDef"],
    HeroPostcardGeneratorDef(HeroPostcardGeneratorDef) => ["CHeroPostcardGeneratorDef"],
    HeroSpecialMovementDef(HeroSpecialMovementDef) => ["CHeroSpecialMovementDef"],
    HeroStatsDef(HeroStatsDef) => ["HERO_STATS"],
    HeroSuitDef(HeroSuitDef) => ["CHeroSuitDef"],
    HeroTitleDef(HeroTitleDef) => ["CHeroTitleDef"],
    HighlightItemDef(HighlightItemDef) => ["CHighlightItemDef"],
    HitLocationDef(HitLocationDef) => ["HIT_LOCATION"],
    HitLocationsDef(HitLocationsDef) => ["CHitLocationsDef"],
    IdleSchedulerDef(IdleSchedulerDef) => ["CIdleSchedulerDef"],
    InterestingToVillagersDef(InterestingToVillagersDef) => ["CInterestingToVillagersDef"],
    InventoryCategoryDef(InventoryCategoryDef) => ["INVENTORY_CATEGORY"],
    InventoryDef(InventoryDef) => ["INVENTORY_TYPE"],
    InventoryItemDef(InventoryItemDef) => ["CInventoryItemDef", "INVENTORY_ITEM"],
    JackDragonDef(JackDragonDef) => ["CJackDragonDef"],
    JackOfBladesBattleDef(JackOfBladesBattleDef) => ["CJackOfBladesBattleDef"],
    KickableDef(KickableDef) => ["CKickableDef"],
    KrakenDef(KrakenDef) => ["CKrakenDef"],
    KrakenTentacleDef(KrakenTentacleDef) => ["CKrakenTentacleDef"],
    LightDef(LightDef) => ["CLightDef"],
    LightningDef(LightningDef) => ["LIGHTNING"],
    LightningOrbDef(LightningOrbDef) => ["CLightningOrbDef"],
    LookDef(LookDef) => ["CLookDef"],
    MaterialDef(MaterialDef) => ["MATERIAL"],
    MazeBattleDef(MazeBattleDef) => ["CMazeBattleDef"],
    MeleeCombatAbilityDef(MeleeCombatAbilityDef) => ["HERO_MELEE_COMBAT_ABILITY"],
    MeleeCombatKnockdownEffects(MeleeCombatKnockdownEffects) => ["MELEE_COMBAT_KNOCKDOWN_EFFECTS"],
    MessageEventDef(MessageEventDef) => ["MESSAGE_EVENT"],
    MultiStaticMeshDef(MultiStaticMeshDef) => ["CMultiStaticMeshDef"],
    NymphDef(NymphDef) => ["CNymphDef"],
    ObjectAugmentationsDef(ObjectAugmentationsDef) => ["CObjectAugmentationsDef"],
    ObjectFamilyDef(ObjectFamilyDef) => ["OBJECT_FAMILY"],
    OccupiableDef(OccupiableDef) => ["COccupiableDef"],
    OpinionDeedEffectsDef(OpinionDeedEffectsDef) => ["OPINION_DEED_EFFECTS"],
    OpinionDeedMaskDef(OpinionDeedMaskDef) => ["OPINION_DEED_MASK"],
    OpinionOfHeroDef(OpinionOfHeroDef) => ["COpinionOfHeroDef"],
    OpinionPersonalityDef(OpinionPersonalityDef) => ["OPINION_PERSONALITY"],
    OpinionReactionManagerDef(OpinionReactionManagerDef) => ["OPINION_REACTION_MANAGER"],
    OpinionReactionMaskDef(OpinionReactionMaskDef) => ["OPINION_REACTION_MASK"],
    OpinionSourceDef(OpinionSourceDef) => ["OPINION_SOURCE"],
    OracleMinigameDef(OracleMinigameDef) => ["COracleMinigameDef"],
    OverheadDisplayDef(OverheadDisplayDef) => ["COverheadDisplayDef"],
    ParticleAttacherDef(ParticleAttacherDef) => ["CParticleAttacherDef"],
    PerceivedThingDef(PerceivedThingDef) => ["CPerceivedThingDef"],
    PhysicsDef(PhysicsDef) => ["CPhysicsDef"],
    PlayerDef(PlayerDef) => ["PLAYER"],
    PlayerGuiDef(PlayerGuiDef) => ["PLAYER_GUI"],
    PlayerInventoryDef(PlayerInventoryDef) => ["PLAYER_INVENTORY"],
    QuestCardDef(QuestCardDef) => ["CQuestCardDef"],
    ReadableDef(ReadableDef) => ["CReadableDef"],
    RegionDef(RegionDef) => ["REGION"],
    RegionScriptDef(RegionScriptDef) => ["CRegionScriptDef"],
    ResurrectionItemDef(ResurrectionItemDef) => ["CResurrectionItemDef"],
    RumbleDef(RumbleDef) => ["CRumbleDef"],
    ScorpionKingBattleDef(ScorpionKingBattleDef) => ["CScorpionKingBattleDef"],
    ScriptDef(ScriptDef) => ["CScriptDef"],
    ShipDef(ShipDef) => ["CShipDef"],
    SimBuildingDef(SimBuildingDef) => ["SIM_BUILDING"],
    SimVoicesDef(SimVoicesDef) => ["SIM_VOICES"],
    SkeletalMorphDef(SkeletalMorphDef) => ["CSkeletalMorphDef"],
    SkyDef(SkyDef) => ["SKY"],
    SmashableDef(SmashableDef) => ["CSmashableDef"],
    SmokeGeneratorDef(SmokeGeneratorDef) => ["CSmokeGeneratorDef"],
    SnowTrollDef(SnowTrollDef) => ["CSnowTrollDef"],
    SoundAtmospheresDef(SoundAtmospheresDef) => ["CSoundAtmospheresDef"],
    SoundDef(SoundDef) => ["SOUND_SETUP"],
    SoundThemeDef(SoundThemeDef) => ["SOUND_THEME"],
    SpecialAbilitiesAssassinRushDef(SpecialAbilitiesAssassinRushDef) => ["SPECIAL_ABILITIES_ASSASSIN_RUSH_DEF"],
    SpecialAbilitiesBattleChargeDef(SpecialAbilitiesBattleChargeDef) => ["SPECIAL_ABILITIES_BATTLE_CHARGE_DEF"],
    SpecialAbilitiesBerserkDef(SpecialAbilitiesBerserkDef) => ["SPECIAL_ABILITIES_BERSERK_DEF"],
    SpecialAbilitiesBulletTimeDef(SpecialAbilitiesBulletTimeDef) => ["SPECIAL_ABILITIES_BULLET_TIME_DEF"],
    SpecialAbilitiesBurntEffectDef(SpecialAbilitiesBurntEffectDef) => ["SPECIAL_ABILITIES_BURNT_EFFECT_DEF"],
    SpecialAbilitiesCreatureTintDef(SpecialAbilitiesCreatureTintDef) => ["SPECIAL_ABILITIES_CREATURE_TINT_DEF"],
    SpecialAbilitiesDrainLifeDef(SpecialAbilitiesDrainLifeDef) => ["SPECIAL_ABILITIES_DRAIN_LIFE_DEF"],
    SpecialAbilitiesDrunkennessDef(SpecialAbilitiesDrunkennessDef) => ["SPECIAL_ABILITIES_DRUNKENNESS_DEF"],
    SpecialAbilitiesElectrocutedEffectDef(SpecialAbilitiesElectrocutedEffectDef) => ["SPECIAL_ABILITIES_ELECTROCUTED_EFFECT_DEF"],
    SpecialAbilitiesEnflameDef(SpecialAbilitiesEnflameDef) => ["SPECIAL_ABILITIES_ENFLAME_DEF"],
    SpecialAbilitiesFireballSpellDef(SpecialAbilitiesFireballSpellDef) => ["SPECIAL_ABILITIES_FIREBALL_SPELL_DEF"],
    SpecialAbilitiesForcePushDataDef(SpecialAbilitiesForcePushDataDef) => ["CSpecialAbilitiesDrainLifeDataDef", "CSpecialAbilitiesForcePushDataDef"],
    SpecialAbilitiesForcePushDef(SpecialAbilitiesForcePushDef) => ["SPECIAL_ABILITIES_FORCE_PUSH_DEF"],
    SpecialAbilitiesGhostSwordDef(SpecialAbilitiesGhostSwordDef) => ["SPECIAL_ABILITIES_GHOST_SWORD_DEF"],
    SpecialAbilitiesHealLifeDef(SpecialAbilitiesHealLifeDef) => ["SPECIAL_ABILITIES_HEAL_LIFE_DEF"],
    SpecialAbilitiesLightningSpellDef(SpecialAbilitiesLightningSpellDef) => ["SPECIAL_ABILITIES_LIGHTNING_SPELL_DEF"],
    SpecialAbilitiesMultiArrowDef(SpecialAbilitiesMultiArrowDef) => ["SPECIAL_ABILITIES_MULTI_ARROW_DEF"],
    SpecialAbilitiesMultiStrikeDef(SpecialAbilitiesMultiStrikeDef) => ["SPECIAL_ABILITIES_MULTI_STRIKE_DEF"],
    SpecialAbilitiesPhysicalShieldDef(SpecialAbilitiesPhysicalShieldDef) => ["SPECIAL_ABILITIES_PHYSICAL_SHIELD_DEF"],
    SpecialAbilitiesSummonSpellDef(SpecialAbilitiesSummonSpellDef) => ["SPECIAL_ABILITIES_SUMMON_SPELL_DEF"],
    SpecialAbilitiesThunderLightningStormDef(SpecialAbilitiesThunderLightningStormDef) => ["SPECIAL_ABILITIES_THUNDER_LIGHTNING_STORM_DEF"],
    SpecialAbilitiesTurncoatSpellDef(SpecialAbilitiesTurncoatSpellDef) => ["SPECIAL_ABILITIES_TURNCOAT_SPELL_DEF"],
    SpecialAbilitiesUnholyPowerDef(SpecialAbilitiesUnholyPowerDef) => ["SPECIAL_ABILITIES_DIVINE_WRATH_DEF", "SPECIAL_ABILITIES_UNHOLY_POWER_DEF"],
    SpecialEffectsDef(SpecialEffectsDef) => ["CSpecialEffectsDef"],
    SpotLightDef(SpotLightDef) => ["CSpotLightDef"],
    StealthDef(StealthDef) => ["CStealthDef"],
    StockItemDef(StockItemDef) => ["CStockItemDef"],
    SummonDef(SummonDef) => ["CSummonDef"],
    SummonableCreatureDef(SummonableCreatureDef) => ["CSummonableCreatureDef"],
    SummonerDef(SummonerDef) => ["CSummonerDef"],
    TCVolumeContainmentTrackerDef(TCVolumeContainmentTrackerDef) => ["CTCVolumeContainmentTrackerDef"],
    TargetingDef(TargetingDef) => ["CTargetingDef"],
    TattooDef(TattooDef) => ["CTattooDef"],
    TavernDef(TavernDef) => ["CTavernDef"],
    TavernGameCardBaseDef(TavernGameCardBaseDef) => ["CTavernGameCardBaseDef"],
    TavernGameCoinBaseDef(TavernGameCoinBaseDef) => ["CTavernGameCoinBaseDef"],
    TavernGameCoinGolfDef(TavernGameCoinGolfDef) => ["CTavernGameCoinGolfDef"],
    TavernGameDef(TavernGameDef) => ["CTavernGameDef"],
    TavernGameShoveHaPennyDef(TavernGameShoveHaPennyDef) => ["CTavernGameShoveHaPennyDef"],
    TavernGameSpotTheAdditionDef(TavernGameSpotTheAdditionDef) => ["CTavernGameSpotTheAdditionDef"],
    TavernTableDef(TavernTableDef) => ["CTavernTableDef"],
    TeleporterDef(TeleporterDef) => ["CTeleporterDef"],
    TextureReplacementDef(TextureReplacementDef) => ["CTextureReplacementDef"],
    ThingBaseDef(ThingBaseDef) => ["THING"],
    ThingBuildingDef(ThingBuildingDef) => ["BUILDING"],
    ThingCreatureDef(ThingCreatureDef) => ["CREATURE"],
    ThingDrainLifeShotDef(ThingDrainLifeShotDef) => ["CThingDrainLifeShotDef"],
    ThingHolySiteDef(ThingHolySiteDef) => ["HOLY_SITE"],
    ThingMarkerDef(ThingMarkerDef) => ["MARKER"],
    ThingMultiArrowShotDef(ThingMultiArrowShotDef) => ["CThingMultiArrowShotDef"],
    ThingNoiseDef(ThingNoiseDef) => ["NOISE"],
    ThingObjectDef(ThingObjectDef) => ["OBJECT"],
    ThingPhysicalSwitchDef(ThingPhysicalSwitchDef) => ["PHYSICAL_SWITCH"],
    ThingShotDef(ThingShotDef) => ["SHOT"],
    ThingSwitchDef(ThingSwitchDef) => ["SWITCH"],
    ThingVillageDef(ThingVillageDef) => ["VILLAGE"],
    ThunderBattleDef(ThunderBattleDef) => ["CThunderBattleDef"],
    TimeAppearanceFadeDef(TimeAppearanceFadeDef) => ["CTimeAppearanceFadeDef"],
    TrapDef(TrapDef) => ["CTrapDef"],
    TrollBattleDef(TrollBattleDef) => ["CTrollBattleDef"],
    TrophyDef(TrophyDef) => ["CTrophyDef"],
    TurncoatDef(TurncoatDef) => ["CTurncoatDef"],
    UILocaleGraphicsDef(UILocaleGraphicsDef) => ["UI_LOCALE_GRAPHICS_DEF"],
    VillageDef(VillageDef) => ["CVillageDef"],
    VillageMemberDef(VillageMemberDef) => ["CVillageMemberDef"],
    VillagePeopleDef(VillagePeopleDef) => ["CVillagePeopleDef"],
    VillagerInteractionsDef(VillagerInteractionsDef) => ["VILLAGER_INTERACTION"],
    WallMountEffectsDef(WallMountEffectsDef) => ["CWallMountEffectsDef"],
    WaspQueenBattleDef(WaspQueenBattleDef) => ["CWaspQueenBattleDef"],
    WeaponDef(WeaponDef) => ["CWeaponDef"],
    WhisperBattleDef(WhisperBattleDef) => ["CWhisperBattleDef"],
    WifeDef(WifeDef) => ["CWifeDef"],
    WillResponseDef(WillResponseDef) => ["CWillResponseDef"],
    DegradableDef(DegradableDef) => ["CDegradableDef"],
    ReplaceableMeshDef(ReplaceableMeshDef) => ["CReplaceableMeshDef"],
    GlobalDef(GlobalDef) => ["GLOBAL"],
    PlayerMovementDef(PlayerMovementDef) => ["PLAYER_MOVEMENT"],
    AppearanceModifierDef(AppearanceModifierDef) => ["CAppearanceModifierDef"],
    ShopDef(ShopDef) => ["CShopDef"],
    ShopItemDef(ShopItemDef) => ["CShopItemDef"],
    Engine(EngineDef) => ["ENGINE"],
    Controls(ControlsDef) => ["CONTROL_SCHEME"],
    FrontEnd(FrontEndDef) => ["FRONT_END"],
    Ui(UiDef) => ["UI"],
    UiIcons(UiIconsDef) => ["UI_ICONS_DEF"],
    UiMiscThings(UiMiscThingsDef) => ["UI_MISC_THINGS_DEF"],
    EngineVideoOptions(EngineVideoOptionsDef) => ["ENGINE_VIDEO_OPTIONS"],
    ConfigOptionsDefaults(ConfigOptionsDefaultsDef) => ["CONFIG_OPTIONS_DEFAULTS_DEF"],
    Environment(EnvironmentDef) => ["CENVIRONMENT_DEF", "ENVIRONMENT"],
    EnvironmentThemeDaySet(EnvironmentThemeDaySetDef) => ["CENVIRONMENT_THEME_DAY", "ENVIRONMENT_THEME_DAY"],
}

macro_rules! sub_def_names {
    ($($name:literal),+ $(,)?) => {
        pub fn def_name_has_subdef_table(name: &str) -> bool {
            match name {
                $( $name => true, )+
                _ => false,
            }
        }
    };
}

sub_def_names! {
    "ARMOUR",
    "ATTACK_PATTERN",
    "BRAIN",
    "BUILDING",
    "CAMERA_MANAGER",
    "CAMERA_MANAGER_SET",
    "CAMERA_MODE",
    "CAreaOfEffectAttackDef",
    "CBalverineBattleDef",
    "CHeroPostcardGeneratorDef",
    "CIdleSchedulerDef",
    "CJackOfBladesBattleDef",
    "CMazeBattleDef",
    "COMBAT_DIALOGUE_DEF",
    "COMBAT_SEQUENCE",
    "COMBAT_TYPE",
    "CONFIG_OPTIONS_DEFAULTS_DEF",
    "CONTROL_SCHEME",
    "CREATURE",
    "CREATURE_ABILITY",
    "CREATURE_GENERATION_FAMILY",
    "CScorpionKingBattleDef",
    "CScriptDef",
    "CCutsceneDef",
    "CRegionScriptDef",
    "CThunderBattleDef",
    "CTrollBattleDef",
    "CWaspQueenBattleDef",
    "CWhisperBattleDef",
    "ENGINE",
    "ENGINE_THEME",
    "ENGINE_THEME_GROUP",
    "ENGINE_VIDEO_OPTIONS",
    "ENVIRONMENT",
    "ENVIRONMENT_THEME_DAY",
    "EXPRESSION",
    "FACTION",
    "FRONT_END",
    "GLOBAL",
    "HERO_ABILITY",
    "HERO_COMBAT",
    "HERO_MELEE_COMBAT_ABILITY",
    "HERO_STATS",
    "HIT_LOCATION",
    "HOLY_SITE",
    "INVENTORY_CATEGORY",
    "INVENTORY_TYPE",
    "LIGHTNING",
    "LOCAL_DETAIL_GENERATOR",
    "MARKER",
    "MATERIAL",
    "MELEE_COMBAT_KNOCKDOWN_EFFECTS",
    "MESSAGE_EVENT",
    "NOISE",
    "OBJECT",
    "OBJECT_FAMILY",
    "OPINION_DEED_EFFECTS",
    "OPINION_DEED_MASK",
    "OPINION_PERSONALITY",
    "OPINION_REACTION_MANAGER",
    "OPINION_REACTION_MASK",
    "OPINION_SOURCE",
    "PHYSICAL_SWITCH",
    "PLAYER",
    "PLAYER_GUI",
    "PLAYER_INVENTORY",
    "PLAYER_MOVEMENT",
    "REGION",
    "SHOT",
    "SIM_BUILDING",
    "SIM_VOICES",
    "SKY",
    "SOUND_SETUP",
    "SOUND_THEME",
    "SPECIAL_ABILITIES_ASSASSIN_RUSH_DEF",
    "SPECIAL_ABILITIES_BATTLE_CHARGE_DEF",
    "SPECIAL_ABILITIES_BERSERK_DEF",
    "SPECIAL_ABILITIES_BULLET_TIME_DEF",
    "SPECIAL_ABILITIES_BURNT_EFFECT_DEF",
    "SPECIAL_ABILITIES_CREATURE_TINT_DEF",
    "SPECIAL_ABILITIES_DIVINE_WRATH_DEF",
    "SPECIAL_ABILITIES_DRAIN_LIFE_DEF",
    "SPECIAL_ABILITIES_DRUNKENNESS_DEF",
    "SPECIAL_ABILITIES_ELECTROCUTED_EFFECT_DEF",
    "SPECIAL_ABILITIES_ENFLAME_DEF",
    "SPECIAL_ABILITIES_FIREBALL_SPELL_DEF",
    "SPECIAL_ABILITIES_FORCE_PUSH_DEF",
    "SPECIAL_ABILITIES_GHOST_SWORD_DEF",
    "SPECIAL_ABILITIES_HEAL_LIFE_DEF",
    "SPECIAL_ABILITIES_LIGHTNING_SPELL_DEF",
    "SPECIAL_ABILITIES_MULTI_ARROW_DEF",
    "SPECIAL_ABILITIES_MULTI_STRIKE_DEF",
    "SPECIAL_ABILITIES_PHYSICAL_SHIELD_DEF",
    "SPECIAL_ABILITIES_SUMMON_SPELL_DEF",
    "SPECIAL_ABILITIES_THUNDER_LIGHTNING_STORM_DEF",
    "SPECIAL_ABILITIES_TURNCOAT_SPELL_DEF",
    "SPECIAL_ABILITIES_UNHOLY_POWER_DEF",
    "SWITCH",
    "THING",
    "THING_GROUP",
    "UI",
    "UI_ICONS_DEF",
    "UI_LOCALE_GRAPHICS_DEF",
    "UI_MISC_THINGS_DEF",
    "VILLAGE",
    "VILLAGER_INTERACTION",
}

/// 3-byte record preamble that precedes each def body. Verified against retail
/// `game.bin`: bodies are `(u32 id, u32 value)` control pairs starting at byte
/// 3, and a body-less entry (e.g. a `CHeroCentreDef` template) is exactly these
/// 3 bytes.
#[derive(Debug, Clone)]
pub struct EntryPreamble {
    pub is_real: bool,
    pub is_template: bool,
    pub unknown_0: u8,
}

impl EntryPreamble {
    fn parse(cur: &mut &[u8]) -> Result<Self, BinParseError> {
        let is_real = take::<u8>(cur).map_err(|e| BinParseError::Field {
            name: "is_real",
            inner: Box::new(e.into()),
        })? == 0x1;
        let is_template = take::<u8>(cur).map_err(|e| BinParseError::Field {
            name: "is_template",
            inner: Box::new(e.into()),
        })? == 0x1;
        let unknown_0 = take::<u8>(cur).map_err(|e| BinParseError::Field {
            name: "unknown_0",
            inner: Box::new(e.into()),
        })?;
        Ok(Self {
            is_real,
            is_template,
            unknown_0,
        })
    }
}

impl EntryPreamble {
    pub fn serialize(&self, out: &mut &mut [u8]) -> Result<(), UnexpectedEnd> {
        put(out, &(self.is_real as u8))?;
        put(out, &(self.is_template as u8))?;
        put(out, &self.unknown_0)?;
        Ok(())
    }

    pub const BYTE_SIZE: usize = 3;
}

#[derive(Debug)]
pub struct DefBinaryEntry<'a> {
    pub global_index: usize,
    pub chunk_index: usize,
    pub chunk_local_index: usize,

    pub name_ref: &'a NameRef,
    pub def_name: Option<&'a str>,
    pub file_name: Option<&'a str>,

    pub record: &'a EntryRecord,
}

impl DefBinary {
    pub fn entries<'a>(
        &'a self,
        names: &'a Names,
    ) -> impl Iterator<Item = DefBinaryEntry<'a>> + 'a {
        self.chunks
            .iter()
            .enumerate()
            .flat_map(move |(chunk_index, chunk)| {
                chunk
                    .entries
                    .iter()
                    .enumerate()
                    .map(move |(chunk_local_index, record)| {
                        let global_index = chunk.entry_base as usize + chunk_local_index;
                        let name_ref = &self.name_refs[global_index];

                        let def_name = names
                            .map
                            .get(&name_ref.def_name_offset)
                            .map(|x| x.string.as_str());

                        let file_name = names
                            .map
                            .get(&name_ref.file_name_offset)
                            .map(|x| x.string.as_str());

                        DefBinaryEntry {
                            global_index,
                            chunk_index,
                            chunk_local_index,
                            name_ref,
                            def_name,
                            file_name,
                            record,
                        }
                    })
            })
    }

    pub fn to_bytes(&self) -> Vec<u8> {
        let chunk_count = self.chunks.len() as u32 + 1;
        let header_size = 13;
        let name_refs_size = self.name_refs.len() * 12;
        let chunk_index_header_size = 8;
        // The index holds `chunk_count` entries: one per chunk, plus a
        // terminating sentinel (verified against retail frontend.bin,
        // game.bin, and script.bin).
        let chunk_index_entries_size = chunk_count as usize * 8;

        let chunk_blobs: Vec<Vec<u8>> = self.chunks.iter().map(|c| c.to_bytes()).collect();
        let chunks_data_size: usize = chunk_blobs.iter().map(|b| b.len()).sum();

        let total_size = header_size
            + name_refs_size
            + chunk_index_header_size
            + chunk_index_entries_size
            + chunks_data_size;

        let mut buf = vec![0u8; total_size];
        let mut cur: &mut [u8] = &mut buf;

        self.header.serialize(&mut cur).unwrap();
        for nr in &self.name_refs {
            nr.serialize(&mut cur).unwrap();
        }
        ChunkIndexHeader {
            chunk_count,
            reserved: 0,
        }
        .serialize(&mut cur)
        .unwrap();

        let mut relative_offset = 0u32;
        let mut cumulative = 0u32;
        for (i, chunk) in self.chunks.iter().enumerate() {
            cumulative += chunk.entry_count;
            ChunkIndexEntry {
                compressed_offset: relative_offset,
                cumulative_entry_count: cumulative,
            }
            .serialize(&mut cur)
            .unwrap();
            relative_offset += chunk_blobs[i].len() as u32;
        }

        // Terminating sentinel entry: both fields hold the total compressed
        // data size (i.e. the end offset of the last chunk). Retail def
        // binaries always carry it, and the game's loader reads it — chunk
        // offsets are relative to a data region that starts *after* the
        // sentinel, so omitting it shifts every chunk seek by 8 bytes and
        // the game crashes on load.
        ChunkIndexEntry {
            compressed_offset: relative_offset,
            cumulative_entry_count: relative_offset,
        }
        .serialize(&mut cur)
        .unwrap();

        for blob in &chunk_blobs {
            put_bytes(&mut cur, blob).unwrap();
        }

        debug_assert!(
            cur.is_empty(),
            "DefBinary::to_bytes: {} bytes remaining",
            cur.len()
        );
        buf
    }
}

pub const ID_BYTE_SIZE: usize = size_of::<u32>();

#[derive(Debug)]
pub enum ParseControlErrorReason {
    MalformedId(TakeError),
    WrongId { expected: u32, found: u32 },
    Wire(ParseWireError),
}

#[derive(Debug)]
pub struct ParseControlError {
    pub name: &'static str,
    pub reason: ParseControlErrorReason,
}

#[derive(Debug)]
pub enum SerializeControlErrorReason {
    MalformedId(UnexpectedEnd),
    Value(UnexpectedEnd),
}

#[derive(Debug)]
pub struct SerializeControlError {
    pub name: &'static str,
    pub reason: SerializeControlErrorReason,
}
