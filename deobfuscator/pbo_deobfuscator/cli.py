"""
CLI (Command Line Interface) para o PBO Deobfuscator.

Uso:
    pbo-deobfuscator info arquivo.pbo          # Mostra informações do PBO
    pbo-deobfuscator extract arquivo.pbo       # Extrai conteúdo
    pbo-deobfuscator deobfuscate arquivo.pbo   # Extrai e desobfusca
    pbo-deobfuscator list arquivo.pbo          # Lista arquivos no PBO
"""

import argparse
import sys
from pathlib import Path

from .pbo_parser import parse_pbo, extract_all, extract_entry_data
from .deobfuscator import PBODeobfuscator


def cmd_info(args: argparse.Namespace) -> int:
    """Mostra informações sobre um arquivo PBO."""
    pbo = parse_pbo(args.pbo_file)

    print(f"\n{'=' * 60}")
    print(f"  PBO DEOBFUSCATOR - ANÁLISE")
    print(f"{'=' * 60}")
    print(f"\n  Arquivo: {pbo.filepath}")
    print(f"  Prefix:  {pbo.prefix or '(nenhum)'}")
    print(f"  Checksum: {pbo.checksum.hex() if pbo.checksum else '(nenhum)'}")
    print(f"\n  Total de arquivos: {pbo.total_files}")
    print(f"  Tamanho total:     {_format_size(pbo.total_size)}")

    if pbo.properties:
        print(f"\n  Propriedades:")
        for key, value in pbo.properties.items():
            print(f"    {key} = {value}")

    print(f"\n  Scripts (.c/.sqf):  {len(pbo.scripts)}")
    print(f"  Configs (.cpp/.hpp): {len(pbo.configs)}")
    print(f"  Texturas (.paa):    {len(pbo.textures)}")
    print(f"  Modelos (.p3d):     {len(pbo.models)}")

    # Detecta sinais de obfuscação
    obfuscation_signs = _detect_obfuscation_signs(pbo)
    if obfuscation_signs:
        print(f"\n  ⚠️  SINAIS DE OBFUSCAÇÃO DETECTADOS:")
        for sign in obfuscation_signs:
            print(f"    • {sign}")

    print(f"\n{'=' * 60}\n")
    return 0


def cmd_list(args: argparse.Namespace) -> int:
    """Lista todos os arquivos dentro do PBO."""
    pbo = parse_pbo(args.pbo_file)

    print(f"\nArquivos em {Path(pbo.filepath).name}:")
    print(f"{'─' * 60}")
    print(f"{'Nome':<45} {'Tamanho':>10}")
    print(f"{'─' * 60}")

    for entry in pbo.entries:
        size_str = _format_size(entry.data_size)
        compressed = " [C]" if entry.is_compressed else ""
        print(f"  {entry.filename:<43} {size_str:>8}{compressed}")

    print(f"{'─' * 60}")
    print(f"  Total: {pbo.total_files} arquivos, {_format_size(pbo.total_size)}")
    return 0


def cmd_extract(args: argparse.Namespace) -> int:
    """Extrai o conteúdo do PBO."""
    pbo = parse_pbo(args.pbo_file)

    output_dir = args.output or Path(args.pbo_file).stem + "_extracted"
    output_path = Path(output_dir)

    print(f"\nExtraindo {pbo.total_files} arquivos para: {output_path}/")
    extracted = extract_all(pbo, output_path)
    print(f"  ✓ {len(extracted)} arquivos extraídos com sucesso!")
    return 0


def cmd_deobfuscate(args: argparse.Namespace) -> int:
    """Extrai e desobfusca o conteúdo do PBO."""
    pbo = parse_pbo(args.pbo_file)

    output_dir = args.output or Path(args.pbo_file).stem + "_deobfuscated"
    output_path = Path(output_dir)
    output_path.mkdir(parents=True, exist_ok=True)

    print(f"\nAnalisando PBO: {Path(pbo.filepath).name}")
    print(f"Prefix: {pbo.prefix or '(nenhum)'}")
    print(f"Total de arquivos: {pbo.total_files}")
    print(f"Scripts para desobfuscar: {len(pbo.scripts)}")
    print()

    # Desobfusca
    deobfuscator = PBODeobfuscator(pbo)
    results = deobfuscator.deobfuscate_all()

    # Salva todos os arquivos (desobfuscados onde possível)
    deobfuscated_filenames = {r.original_filename for r in results}
    ignored_files: set[str] = set()

    # Detecta arquivos que são alvos de redirect (não precisam ser extraídos separadamente)
    for result in results:
        if result.redirect_target:
            # O arquivo alvo do redirect pode estar com path diferente
            ignored_files.add(result.redirect_target)

    # Salva scripts desobfuscados
    for result in results:
        rel_path = result.original_filename.replace('\\', '/')
        file_path = output_path / rel_path
        file_path.parent.mkdir(parents=True, exist_ok=True)
        file_path.write_text(result.deobfuscated_content, encoding='utf-8')

    # Salva outros arquivos (não-scripts)
    for entry in pbo.entries:
        if entry.filename in deobfuscated_filenames:
            continue
        rel_path = entry.filename.replace('\\', '/')
        file_path = output_path / rel_path
        file_path.parent.mkdir(parents=True, exist_ok=True)
        data = extract_entry_data(pbo, entry)
        file_path.write_bytes(data)

    # Mostra relatório
    print(deobfuscator.get_report())

    # Salva relatório
    report_path = output_path / "_DEOBFUSCATION_REPORT.txt"
    report_path.write_text(deobfuscator.get_report(), encoding='utf-8')

    print(f"\nArquivos salvos em: {output_path}/")
    print(f"Relatório: {report_path}")
    return 0


def _detect_obfuscation_signs(pbo) -> list[str]:
    """Detecta sinais de que o PBO foi obfuscado."""
    signs = []

    # Arquivos com nomes suspeitos (hashes, random)
    suspicious_names = []
    for entry in pbo.entries:
        name = Path(entry.filename).stem
        if len(name) > 20 and all(c in '0123456789abcdef' for c in name.lower()):
            suspicious_names.append(entry.filename)

    if suspicious_names:
        signs.append(f"{len(suspicious_names)} arquivo(s) com nomes tipo hash")

    # Scripts muito pequenos (possíveis redirects)
    tiny_scripts = [e for e in pbo.scripts if e.data_size < 100]
    if tiny_scripts and len(tiny_scripts) > len(pbo.scripts) * 0.5:
        signs.append(f"{len(tiny_scripts)} scripts muito pequenos (possíveis redirects)")

    # Verifica conteúdo dos scripts pequenos
    for entry in tiny_scripts[:5]:
        data = extract_entry_data(pbo, entry)
        if b'#include' in data and len(data) < 100:
            signs.append("Scripts contêm apenas #include (obfuscação por redirect)")
            break

    # Muitos arquivos em paths profundos/estranhos
    deep_paths = [e for e in pbo.entries if e.filename.count('\\') > 5]
    if deep_paths:
        signs.append(f"{len(deep_paths)} arquivo(s) em paths muito profundos")

    return signs


def _format_size(size: int) -> str:
    """Formata tamanho em bytes para formato legível."""
    if size < 1024:
        return f"{size} B"
    elif size < 1024 * 1024:
        return f"{size / 1024:.1f} KB"
    elif size < 1024 * 1024 * 1024:
        return f"{size / (1024 * 1024):.1f} MB"
    return f"{size / (1024 * 1024 * 1024):.1f} GB"


def main() -> int:
    """Entry point do CLI."""
    parser = argparse.ArgumentParser(
        prog='pbo-deobfuscator',
        description='Desobfuscador de arquivos PBO para DayZ e ArmA 3',
        epilog='Inspirado no pbo.tools - Ferramenta para abrir e desobfuscar PBOs protegidos.'
    )

    subparsers = parser.add_subparsers(dest='command', help='Comandos disponíveis')

    # info
    info_parser = subparsers.add_parser('info', help='Mostra informações do PBO')
    info_parser.add_argument('pbo_file', help='Caminho para o arquivo .pbo')

    # list
    list_parser = subparsers.add_parser('list', help='Lista arquivos no PBO')
    list_parser.add_argument('pbo_file', help='Caminho para o arquivo .pbo')

    # extract
    extract_parser = subparsers.add_parser('extract', help='Extrai conteúdo do PBO')
    extract_parser.add_argument('pbo_file', help='Caminho para o arquivo .pbo')
    extract_parser.add_argument('-o', '--output', help='Diretório de saída')

    # deobfuscate
    deobf_parser = subparsers.add_parser(
        'deobfuscate', help='Extrai e desobfusca o PBO',
        aliases=['deobf', 'd']
    )
    deobf_parser.add_argument('pbo_file', help='Caminho para o arquivo .pbo')
    deobf_parser.add_argument('-o', '--output', help='Diretório de saída')

    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        return 1

    try:
        commands = {
            'info': cmd_info,
            'list': cmd_list,
            'extract': cmd_extract,
            'deobfuscate': cmd_deobfuscate,
            'deobf': cmd_deobfuscate,
            'd': cmd_deobfuscate,
        }
        return commands[args.command](args)
    except FileNotFoundError as e:
        print(f"\nErro: {e}", file=sys.stderr)
        return 1
    except ValueError as e:
        print(f"\nErro no formato PBO: {e}", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"\nErro inesperado: {e}", file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
