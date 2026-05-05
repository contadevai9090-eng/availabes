"""
Parser binário para arquivos PBO (Packed Bank of Objects).

Formato PBO:
- Header: entradas contíguas de 21+ bytes (filename, packing, original_size, reserved, timestamp, data_size)
- Primeira entrada pode ser "properties" (packing_method = 0x56657273 / "Vers")
- Entrada vazia (filename = "") marca fim do header
- Bloco de dados contíguo
- (Opcional) Checksum SHA1 de 21 bytes no final
"""

import struct
import io
from dataclasses import dataclass, field
from pathlib import Path
from typing import BinaryIO


# Constantes do formato PBO
PACKING_METHOD_UNCOMPRESSED = 0x00000000
PACKING_METHOD_COMPRESSED = 0x43707273  # "Cprs"
PACKING_METHOD_VERSION = 0x56657273     # "Vers" - marca entrada de properties


@dataclass
class PBOEntry:
    """Representa uma entrada (arquivo) dentro do PBO."""
    filename: str
    packing_method: int
    original_size: int
    reserved: int
    timestamp: int
    data_size: int
    data_offset: int = 0

    @property
    def is_compressed(self) -> bool:
        return self.packing_method == PACKING_METHOD_COMPRESSED

    @property
    def is_version_entry(self) -> bool:
        return self.packing_method == PACKING_METHOD_VERSION

    def __repr__(self) -> str:
        status = "compressed" if self.is_compressed else "raw"
        return f"PBOEntry({self.filename!r}, {status}, size={self.data_size})"


@dataclass
class PBOFile:
    """Representa um arquivo PBO parseado."""
    filepath: str
    properties: dict = field(default_factory=dict)
    entries: list[PBOEntry] = field(default_factory=list)
    prefix: str = ""
    checksum: bytes = b""
    _raw_data: bytes = b""

    @property
    def total_files(self) -> int:
        return len(self.entries)

    @property
    def total_size(self) -> int:
        return sum(e.data_size for e in self.entries)

    @property
    def scripts(self) -> list[PBOEntry]:
        return [e for e in self.entries if e.filename.lower().endswith('.c') or
                e.filename.lower().endswith('.sqf')]

    @property
    def configs(self) -> list[PBOEntry]:
        return [e for e in self.entries if
                e.filename.lower() in ('config.cpp', 'config.bin') or
                e.filename.lower().endswith('.cpp') or
                e.filename.lower().endswith('.hpp')]

    @property
    def textures(self) -> list[PBOEntry]:
        return [e for e in self.entries if
                e.filename.lower().endswith('.paa') or
                e.filename.lower().endswith('.pac')]

    @property
    def models(self) -> list[PBOEntry]:
        return [e for e in self.entries if e.filename.lower().endswith('.p3d')]


def _read_asciiz(stream: BinaryIO) -> str:
    """Lê uma string null-terminated do stream."""
    chars = []
    while True:
        byte = stream.read(1)
        if not byte or byte == b'\x00':
            break
        chars.append(byte)
    return b''.join(chars).decode('latin-1')


def _read_entry(stream: BinaryIO) -> PBOEntry:
    """Lê uma entrada do header PBO."""
    filename = _read_asciiz(stream)
    packing_method, original_size, reserved, timestamp, data_size = struct.unpack(
        '<IIIII', stream.read(20)
    )
    return PBOEntry(
        filename=filename,
        packing_method=packing_method,
        original_size=original_size,
        reserved=reserved,
        timestamp=timestamp,
        data_size=data_size,
    )


def _read_properties(stream: BinaryIO) -> dict:
    """Lê as propriedades do header (pares key=value null-terminated)."""
    properties = {}
    while True:
        key = _read_asciiz(stream)
        if not key:
            break
        value = _read_asciiz(stream)
        properties[key] = value
    return properties


def parse_pbo(filepath: str | Path) -> PBOFile:
    """
    Faz o parsing completo de um arquivo PBO.

    Args:
        filepath: Caminho para o arquivo .pbo

    Returns:
        PBOFile com todas as entradas e metadados

    Raises:
        FileNotFoundError: Se o arquivo não existir
        ValueError: Se o formato for inválido
    """
    filepath = Path(filepath)
    if not filepath.exists():
        raise FileNotFoundError(f"Arquivo PBO não encontrado: {filepath}")

    raw_data = filepath.read_bytes()
    stream = io.BytesIO(raw_data)
    pbo = PBOFile(filepath=str(filepath), _raw_data=raw_data)

    # Lê primeira entrada para verificar se é properties header
    first_entry = _read_entry(stream)

    if first_entry.is_version_entry:
        # Primeira entrada é o header de properties
        pbo.properties = _read_properties(stream)
        pbo.prefix = pbo.properties.get('prefix', '')
    else:
        # Não tem properties - primeira entrada é um arquivo real
        if first_entry.filename:
            pbo.entries.append(first_entry)

    # Lê demais entradas até encontrar entrada vazia
    while True:
        entry = _read_entry(stream)
        if not entry.filename:
            break
        pbo.entries.append(entry)

    # Calcula offset dos dados para cada entrada
    data_start = stream.tell()
    current_offset = data_start

    for entry in pbo.entries:
        entry.data_offset = current_offset
        current_offset += entry.data_size

    # Tenta ler checksum SHA1 (últimos 21 bytes: 1 byte zero + 20 bytes SHA)
    checksum_start = current_offset
    if checksum_start + 21 <= len(raw_data):
        if raw_data[checksum_start] == 0x00:
            pbo.checksum = raw_data[checksum_start + 1:checksum_start + 21]

    return pbo


def extract_entry_data(pbo: PBOFile, entry: PBOEntry) -> bytes:
    """
    Extrai os dados brutos de uma entrada do PBO.

    Args:
        pbo: O PBOFile parseado
        entry: A entrada a ser extraída

    Returns:
        bytes com o conteúdo do arquivo
    """
    data = pbo._raw_data[entry.data_offset:entry.data_offset + entry.data_size]

    if entry.is_compressed and entry.original_size != entry.data_size:
        data = _decompress_lzss(data, entry.original_size)

    return data


def _decompress_lzss(compressed: bytes, output_size: int) -> bytes:
    """
    Descomprime dados usando LZSS (método Apple usado pelo Bohemia Interactive).

    O algoritmo usa um buffer circular de 4096 bytes e pacotes com flag bits
    que indicam se o próximo dado é literal (1) ou referência (0).
    """
    output = bytearray()
    stream = io.BytesIO(compressed)

    # Buffer circular de 4096 bytes preenchido com espaços
    ring_buffer = bytearray(b'\x20' * 4096)
    ring_pos = 0

    while len(output) < output_size:
        flag_byte_data = stream.read(1)
        if not flag_byte_data:
            break
        flag_byte = flag_byte_data[0]

        for bit_idx in range(8):
            if len(output) >= output_size:
                break

            if flag_byte & (1 << bit_idx):
                # Bit = 1: byte literal
                byte_data = stream.read(1)
                if not byte_data:
                    break
                byte = byte_data[0]
                output.append(byte)
                ring_buffer[ring_pos] = byte
                ring_pos = (ring_pos + 1) % 4096
            else:
                # Bit = 0: referência (2 bytes)
                ref_data = stream.read(2)
                if len(ref_data) < 2:
                    break
                b1, b2 = ref_data[0], ref_data[1]

                # Formato: offset (12 bits) + length (4 bits)
                offset = b1 | ((b2 & 0xF0) << 4)
                length = (b2 & 0x0F) + 3

                for i in range(length):
                    if len(output) >= output_size:
                        break
                    byte = ring_buffer[(offset + i) % 4096]
                    output.append(byte)
                    ring_buffer[ring_pos] = byte
                    ring_pos = (ring_pos + 1) % 4096

    return bytes(output[:output_size])


def extract_all(pbo: PBOFile, output_dir: str | Path) -> list[Path]:
    """
    Extrai todos os arquivos do PBO para um diretório.

    Args:
        pbo: O PBOFile parseado
        output_dir: Diretório de saída

    Returns:
        Lista de caminhos dos arquivos extraídos
    """
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    extracted = []

    for entry in pbo.entries:
        # Normaliza path (PBO usa backslash)
        rel_path = entry.filename.replace('\\', '/')
        file_path = output_dir / rel_path

        file_path.parent.mkdir(parents=True, exist_ok=True)

        data = extract_entry_data(pbo, entry)
        file_path.write_bytes(data)
        extracted.append(file_path)

    return extracted
