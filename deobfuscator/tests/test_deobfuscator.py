"""Testes para o engine de desobfuscação."""
import struct
import tempfile
from pathlib import Path

from pbo_deobfuscator.pbo_parser import parse_pbo, PACKING_METHOD_UNCOMPRESSED, PACKING_METHOD_VERSION
from pbo_deobfuscator.deobfuscator import PBODeobfuscator


def _create_test_pbo(files: dict[str, bytes], prefix: str = "") -> bytes:
    """Cria um PBO binário de teste."""
    buffer = bytearray()

    if prefix:
        buffer += b'\x00'
        buffer += struct.pack('<IIIII', PACKING_METHOD_VERSION, 0, 0, 0, 0)
        buffer += b'prefix\x00'
        buffer += prefix.encode('latin-1') + b'\x00'
        buffer += b'\x00'

    for filename, data in files.items():
        buffer += filename.encode('latin-1') + b'\x00'
        buffer += struct.pack('<IIIII',
                             PACKING_METHOD_UNCOMPRESSED,
                             len(data), 0, 0, len(data))

    buffer += b'\x00'
    buffer += struct.pack('<IIIII', 0, 0, 0, 0, 0)

    for data in files.values():
        buffer += data

    return bytes(buffer)


def test_deobfuscate_include_redirect():
    """Testa desobfuscação de include redirect."""
    real_code = b'void Init() {\n  Print("Hello World");\n}\n'
    files = {
        'init.c': b'#include "hidden\\real_init.c"\r\n',
        'hidden\\real_init.c': real_code,
    }
    pbo_data = _create_test_pbo(files, prefix="")

    with tempfile.NamedTemporaryFile(suffix='.pbo', delete=False) as f:
        f.write(pbo_data)
        f.flush()

        pbo = parse_pbo(f.name)
        deobf = PBODeobfuscator(pbo)
        results = deobf.deobfuscate_all()

        # init.c deveria ter sido resolvido para o conteúdo real
        init_result = next(r for r in results if r.original_filename == 'init.c')
        assert 'Print("Hello World")' in init_result.deobfuscated_content
        assert init_result.was_obfuscated
        assert any('redirect' in c.lower() for c in init_result.changes_made)

    Path(f.name).unlink()


def test_deobfuscate_dead_code():
    """Testa remoção de dead code."""
    code_with_dead = b'void Main() {\n  if(false){junk();}\n  Print("real");\n}\n'
    files = {'main.c': code_with_dead}
    pbo_data = _create_test_pbo(files)

    with tempfile.NamedTemporaryFile(suffix='.pbo', delete=False) as f:
        f.write(pbo_data)
        f.flush()

        pbo = parse_pbo(f.name)
        deobf = PBODeobfuscator(pbo)
        results = deobf.deobfuscate_all()

        result = results[0]
        assert 'if(false)' not in result.deobfuscated_content
        assert 'Print("real")' in result.deobfuscated_content

    Path(f.name).unlink()


def test_deobfuscate_string_concat():
    """Testa desobfuscação de strings concatenadas."""
    code = b'string msg = "H" + "e" + "l" + "l" + "o";\n'
    files = {'test.c': code}
    pbo_data = _create_test_pbo(files)

    with tempfile.NamedTemporaryFile(suffix='.pbo', delete=False) as f:
        f.write(pbo_data)
        f.flush()

        pbo = parse_pbo(f.name)
        deobf = PBODeobfuscator(pbo)
        results = deobf.deobfuscate_all()

        result = results[0]
        assert '"Hello"' in result.deobfuscated_content

    Path(f.name).unlink()


def test_deobfuscate_minified_code():
    """Testa reformatação de código minificado."""
    # Código todo em uma linha
    code = b'void Main(){int x=1;if(x>0){Print("yes");}else{Print("no");}}'
    files = {'minified.c': code}
    pbo_data = _create_test_pbo(files)

    with tempfile.NamedTemporaryFile(suffix='.pbo', delete=False) as f:
        f.write(pbo_data)
        f.flush()

        pbo = parse_pbo(f.name)
        deobf = PBODeobfuscator(pbo)
        results = deobf.deobfuscate_all()

        result = results[0]
        # Código deve ter sido reformatado com múltiplas linhas
        lines = result.deobfuscated_content.strip().split('\n')
        assert len(lines) > 1

    Path(f.name).unlink()


def test_report_generation():
    """Testa geração de relatório."""
    files = {
        'init.c': b'#include "hidden\\code.c"\r\n',
        'hidden\\code.c': b'void Init() {}',
    }
    pbo_data = _create_test_pbo(files, prefix="TestMod")

    with tempfile.NamedTemporaryFile(suffix='.pbo', delete=False) as f:
        f.write(pbo_data)
        f.flush()

        pbo = parse_pbo(f.name)
        deobf = PBODeobfuscator(pbo)
        deobf.deobfuscate_all()

        report = deobf.get_report()
        assert "RELATÓRIO" in report
        assert "TestMod" in report

    Path(f.name).unlink()


if __name__ == '__main__':
    test_deobfuscate_include_redirect()
    test_deobfuscate_dead_code()
    test_deobfuscate_string_concat()
    test_deobfuscate_minified_code()
    test_report_generation()
    print("Todos os testes de desobfuscação passaram!")
