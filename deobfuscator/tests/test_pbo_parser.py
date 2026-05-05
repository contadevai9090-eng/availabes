"""Testes para o parser de PBO."""
import struct
import tempfile
from pathlib import Path

from pbo_deobfuscator.pbo_parser import (
    parse_pbo,
    extract_entry_data,
    extract_all,
    PBOEntry,
    PACKING_METHOD_UNCOMPRESSED,
    PACKING_METHOD_VERSION,
)


def _create_test_pbo(files: dict[str, bytes], prefix: str = "") -> bytes:
    """Cria um PBO binário de teste em memória."""
    buffer = bytearray()

    # Properties header (Vers entry)
    if prefix:
        buffer += b'\x00'  # filename vazio para properties entry
        buffer += struct.pack('<IIIII', PACKING_METHOD_VERSION, 0, 0, 0, 0)
        # Properties: prefix
        buffer += b'prefix\x00'
        buffer += prefix.encode('latin-1') + b'\x00'
        buffer += b'\x00'  # fim das properties

    # File entries
    for filename, data in files.items():
        buffer += filename.encode('latin-1') + b'\x00'
        buffer += struct.pack('<IIIII',
                             PACKING_METHOD_UNCOMPRESSED,
                             len(data),  # original_size
                             0,          # reserved
                             0,          # timestamp
                             len(data))  # data_size

    # Empty entry (end of header)
    buffer += b'\x00'
    buffer += struct.pack('<IIIII', 0, 0, 0, 0, 0)

    # Data block
    for data in files.values():
        buffer += data

    return bytes(buffer)


def test_parse_simple_pbo():
    """Testa parsing de um PBO simples sem obfuscação."""
    files = {
        'config.cpp': b'class CfgPatches { class MyMod {}; };',
        'scripts\\init.c': b'void Init() { Print("Hello"); }',
    }
    pbo_data = _create_test_pbo(files, prefix="MyMod")

    with tempfile.NamedTemporaryFile(suffix='.pbo', delete=False) as f:
        f.write(pbo_data)
        f.flush()

        pbo = parse_pbo(f.name)

        assert pbo.prefix == "MyMod"
        assert pbo.total_files == 2
        assert len(pbo.scripts) == 1
        assert len(pbo.configs) == 1

        # Verifica conteúdo
        config_entry = pbo.entries[0]
        assert config_entry.filename == 'config.cpp'
        data = extract_entry_data(pbo, config_entry)
        assert data == b'class CfgPatches { class MyMod {}; };'

    Path(f.name).unlink()


def test_parse_pbo_without_properties():
    """Testa parsing de PBO sem header de properties."""
    files = {
        'mission.sqm': b'version=12;',
        'init.sqf': b'hint "loaded";',
    }
    pbo_data = _create_test_pbo(files, prefix="")

    with tempfile.NamedTemporaryFile(suffix='.pbo', delete=False) as f:
        f.write(pbo_data)
        f.flush()

        pbo = parse_pbo(f.name)

        assert pbo.prefix == ""
        assert pbo.total_files == 2

    Path(f.name).unlink()


def test_extract_all():
    """Testa extração de todos os arquivos."""
    files = {
        'config.cpp': b'class CfgPatches {};',
        'scripts\\main.c': b'void Main() {}',
        'textures\\logo.paa': b'\x00\x01\x02\x03',
    }
    pbo_data = _create_test_pbo(files, prefix="TestMod")

    with tempfile.NamedTemporaryFile(suffix='.pbo', delete=False) as f:
        f.write(pbo_data)
        f.flush()

        pbo = parse_pbo(f.name)

        with tempfile.TemporaryDirectory() as out_dir:
            extracted = extract_all(pbo, out_dir)
            assert len(extracted) == 3

            # Verifica estrutura de pastas
            assert (Path(out_dir) / 'config.cpp').exists()
            assert (Path(out_dir) / 'scripts' / 'main.c').exists()
            assert (Path(out_dir) / 'textures' / 'logo.paa').exists()

            # Verifica conteúdo
            assert (Path(out_dir) / 'config.cpp').read_bytes() == b'class CfgPatches {};'

    Path(f.name).unlink()


def test_pbo_entry_properties():
    """Testa propriedades da PBOEntry."""
    entry = PBOEntry(
        filename='test.c',
        packing_method=PACKING_METHOD_UNCOMPRESSED,
        original_size=100,
        reserved=0,
        timestamp=0,
        data_size=100,
    )
    assert not entry.is_compressed
    assert not entry.is_version_entry
    assert 'test.c' in repr(entry)


def test_include_redirect_detection():
    """Testa detecção de scripts com include redirect."""
    files = {
        'init.c': b'#include "MyMod\\hidden\\real_init.c"\r\n',
        'hidden\\real_init.c': b'void Init() {\n  Print("real code");\n}\n',
    }
    pbo_data = _create_test_pbo(files, prefix="MyMod")

    with tempfile.NamedTemporaryFile(suffix='.pbo', delete=False) as f:
        f.write(pbo_data)
        f.flush()

        pbo = parse_pbo(f.name)
        assert len(pbo.scripts) == 2

    Path(f.name).unlink()


if __name__ == '__main__':
    test_parse_simple_pbo()
    test_parse_pbo_without_properties()
    test_extract_all()
    test_pbo_entry_properties()
    test_include_redirect_detection()
    print("Todos os testes passaram!")
