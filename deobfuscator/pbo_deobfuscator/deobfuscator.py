"""
Engine de desobfuscação para scripts DayZ/ArmA.

Reverte técnicas de obfuscação comuns:
- Include-based obfuscation (arquivos que redirecionam via #include)
- Código minificado (reformata e indenta)
- Variáveis renomeadas (detecta padrões e sugere nomes)
- Dead code / junk code injection
- String obfuscation (encoding, splitting, reversing)
- Path obfuscation
"""

import re
from dataclasses import dataclass, field
from pathlib import Path

from .pbo_parser import PBOFile, PBOEntry, extract_entry_data


@dataclass
class DeobfuscationResult:
    """Resultado da desobfuscação de um arquivo."""
    original_filename: str
    deobfuscated_content: str
    original_content: str
    changes_made: list[str] = field(default_factory=list)
    was_obfuscated: bool = False
    redirect_target: str = ""


# Regex para detectar obfuscação por include redirect
INCLUDE_REDIRECT_RE = re.compile(
    rb'^(?:(?://[^\r\n]*|/\*(?:\*(?!/)|[^*])*\*/)\r?\n)?'
    rb'#include\s+"([^"]+)"(?:\r?\n)?$'
)

# Padrões de variáveis ofuscadas (nomes aleatórios curtos)
OBFUSCATED_VAR_PATTERNS = [
    re.compile(r'\b([a-z]{1,2}\d{1,4})\b'),          # a1, bc23
    re.compile(r'\b(_[a-zA-Z0-9]{1,3})\b'),           # _a, _x1
    re.compile(r'\b([A-Z]{2,5}\d{2,5})\b'),           # AB12, XYZ123
    re.compile(r'\b(_0x[a-fA-F0-9]{4,8})\b'),         # _0xDEAD
]

# Padrões de dead code (código que não faz nada)
DEAD_CODE_PATTERNS = [
    re.compile(r'if\s*\(\s*false\s*\)\s*\{[^}]*\}', re.DOTALL),
    re.compile(r'if\s*\(\s*0\s*\)\s*\{[^}]*\}', re.DOTALL),
    re.compile(r'while\s*\(\s*false\s*\)\s*\{[^}]*\}', re.DOTALL),
    re.compile(r'//\s*[a-f0-9]{8,}\s*$', re.MULTILINE),  # Hex comments
    re.compile(r'/\*\s*[a-f0-9]{16,}\s*\*/', re.DOTALL),  # Hex block comments
]

# Strings ofuscadas comuns
STRING_OBFUSCATION_PATTERNS = [
    # String.fromCharCode ou equivalentes
    re.compile(r'""\.Join\(\s*""\s*,\s*\[([^\]]+)\]\s*\)'),
    # Concatenação excessiva de chars
    re.compile(r'(".")\s*\+\s*(".")\s*(?:\+\s*("."))*'),
]

# Keywords DayZ que NUNCA devem ser modificadas
DAYZ_KEYWORDS = {
    'void', 'int', 'float', 'bool', 'string', 'vector', 'class', 'extends',
    'modded', 'override', 'private', 'protected', 'static', 'const', 'ref',
    'autoptr', 'auto', 'new', 'delete', 'null', 'NULL', 'true', 'false',
    'if', 'else', 'for', 'foreach', 'while', 'do', 'switch', 'case',
    'default', 'break', 'continue', 'return', 'this', 'super', 'typename',
    'typedef', 'enum', 'proto', 'native', 'volatile', 'out', 'inout',
    'owned', 'sealed', 'event', 'array', 'set', 'map',
}


class PBODeobfuscator:
    """Engine principal de desobfuscação."""

    def __init__(self, pbo: PBOFile):
        self.pbo = pbo
        self.results: list[DeobfuscationResult] = []
        self._file_cache: dict[str, bytes] = {}

    def _get_file_content(self, filename: str) -> bytes | None:
        """Busca conteúdo de um arquivo no PBO (cache)."""
        if filename in self._file_cache:
            return self._file_cache[filename]

        # Normaliza separadores
        normalized = filename.replace('/', '\\')
        normalized_fwd = filename.replace('\\', '/')

        for entry in self.pbo.entries:
            entry_norm = entry.filename.replace('/', '\\')
            entry_fwd = entry.filename.replace('\\', '/')
            if entry_norm == normalized or entry_fwd == normalized_fwd:
                data = extract_entry_data(self.pbo, entry)
                self._file_cache[filename] = data
                return data

        return None

    def deobfuscate_all(self) -> list[DeobfuscationResult]:
        """Desobfusca todos os scripts do PBO."""
        self.results = []

        for entry in self.pbo.entries:
            if self._is_script(entry.filename):
                result = self.deobfuscate_entry(entry)
                self.results.append(result)

        return self.results

    def deobfuscate_entry(self, entry: PBOEntry) -> DeobfuscationResult:
        """Desobfusca uma entrada específica."""
        raw_data = extract_entry_data(self.pbo, entry)
        original_content = raw_data.decode('latin-1', errors='replace')

        result = DeobfuscationResult(
            original_filename=entry.filename,
            original_content=original_content,
            deobfuscated_content=original_content,
        )

        # 1. Verifica obfuscação por include redirect
        content = self._resolve_include_redirect(raw_data, entry, result)

        # 2. Remove dead code
        content = self._remove_dead_code(content, result)

        # 3. Desobfusca strings
        content = self._deobfuscate_strings(content, result)

        # 4. Reformata código minificado
        content = self._beautify_code(content, result)

        # 5. Detecta e anota variáveis ofuscadas
        content = self._annotate_obfuscated_vars(content, result)

        result.deobfuscated_content = content
        result.was_obfuscated = len(result.changes_made) > 0

        return result

    def _resolve_include_redirect(
        self, raw_data: bytes, entry: PBOEntry, result: DeobfuscationResult
    ) -> str:
        """Resolve obfuscação baseada em #include redirect."""
        match = INCLUDE_REDIRECT_RE.match(raw_data)
        if not match:
            return raw_data.decode('latin-1', errors='replace')

        target_path = match.group(1).decode('latin-1')
        result.redirect_target = target_path

        # Remove prefix do PBO se presente
        if self.pbo.prefix:
            prefix_with_sep = self.pbo.prefix.replace('/', '\\') + '\\'
            if target_path.replace('/', '\\').startswith(prefix_with_sep):
                target_path = target_path[len(prefix_with_sep):]

        # Busca o arquivo alvo
        target_data = self._get_file_content(target_path)
        if target_data is None:
            # Tenta variações de path
            for variant in self._path_variants(target_path):
                target_data = self._get_file_content(variant)
                if target_data is not None:
                    break

        if target_data is not None:
            result.changes_made.append(
                f"Resolvido include redirect: {entry.filename} -> {target_path}"
            )
            # Recursivamente resolve se o alvo também é um redirect
            sub_match = INCLUDE_REDIRECT_RE.match(target_data)
            if sub_match:
                return self._resolve_include_redirect(target_data, entry, result)
            return target_data.decode('latin-1', errors='replace')

        return raw_data.decode('latin-1', errors='replace')

    def _path_variants(self, path: str) -> list[str]:
        """Gera variações de um path para busca."""
        variants = []
        # Troca separadores
        variants.append(path.replace('/', '\\'))
        variants.append(path.replace('\\', '/'))
        # Remove prefixo se existir
        parts = path.replace('\\', '/').split('/')
        if len(parts) > 1:
            variants.append('/'.join(parts[1:]))
            variants.append('\\'.join(parts[1:]))
        return variants

    def _remove_dead_code(self, content: str, result: DeobfuscationResult) -> str:
        """Remove dead code injetado pela obfuscação."""
        original = content
        for pattern in DEAD_CODE_PATTERNS:
            matches = pattern.findall(content)
            if matches:
                content = pattern.sub('', content)

        if content != original:
            result.changes_made.append("Removido dead code / junk code")

        return content

    def _deobfuscate_strings(self, content: str, result: DeobfuscationResult) -> str:
        """Tenta reverter obfuscação de strings."""
        original = content

        # Reverte concatenação de caracteres individuais
        # "h" + "e" + "l" + "l" + "o" -> "hello"
        pattern = re.compile(r'"([^"])"(?:\s*\+\s*"([^"])")+')

        def join_chars(match: re.Match) -> str:
            full = match.group(0)
            chars = re.findall(r'"([^"])"', full)
            return '"' + ''.join(chars) + '"'

        content = pattern.sub(join_chars, content)

        # Reverte ToString de arrays de char codes
        # Exemplo: String(array[65, 66, 67]) -> "ABC"
        charcode_pattern = re.compile(
            r'(?:String\.FromCharCode|chr)\s*\(\s*(\d+(?:\s*,\s*\d+)*)\s*\)'
        )

        def decode_charcodes(match: re.Match) -> str:
            codes = [int(c.strip()) for c in match.group(1).split(',')]
            try:
                return '"' + ''.join(chr(c) for c in codes) + '"'
            except (ValueError, OverflowError):
                return match.group(0)

        content = charcode_pattern.sub(decode_charcodes, content)

        # Reverte strings invertidas
        # Exemplo: "olleh".Reverse() -> "hello"
        reverse_pattern = re.compile(r'"([^"]+)"\.Reverse\(\)')

        def unreverse(match: re.Match) -> str:
            return '"' + match.group(1)[::-1] + '"'

        content = reverse_pattern.sub(unreverse, content)

        if content != original:
            result.changes_made.append("Desobfuscação de strings aplicada")

        return content

    def _beautify_code(self, content: str, result: DeobfuscationResult) -> str:
        """Reformata código minificado para leitura humana."""
        # Detecta se o código está minificado (linhas muito longas)
        lines = content.split('\n')
        avg_line_length = sum(len(l) for l in lines) / max(len(lines), 1)

        if avg_line_length < 200 and len(lines) > 3:
            return content  # Já está formatado

        original = content

        # Adiciona quebras de linha após ; { }
        content = re.sub(r';\s*(?![\s\n])', ';\n', content)
        content = re.sub(r'\{\s*(?![\s\n])', '{\n', content)
        content = re.sub(r'\}\s*(?![\s\n;])', '}\n', content)

        # Indentação básica
        content = self._indent_code(content)

        if content != original:
            result.changes_made.append("Código reformatado (beautify)")

        return content

    def _indent_code(self, content: str) -> str:
        """Aplica indentação baseada em { e }."""
        lines = content.split('\n')
        indented = []
        level = 0
        indent = '\t'

        for line in lines:
            stripped = line.strip()
            if not stripped:
                indented.append('')
                continue

            # Reduz indent antes de }
            close_count = stripped.count('}') - stripped.count('{')
            if stripped.startswith('}') or (close_count > 0 and not stripped.startswith('{')):
                level = max(0, level - 1)

            indented.append(indent * level + stripped)

            # Aumenta indent após {
            open_count = stripped.count('{') - stripped.count('}')
            if open_count > 0:
                level += open_count

        return '\n'.join(indented)

    def _annotate_obfuscated_vars(self, content: str, result: DeobfuscationResult) -> str:
        """Detecta variáveis com nomes ofuscados e adiciona comentários."""
        obfuscated_vars = set()

        for pattern in OBFUSCATED_VAR_PATTERNS:
            matches = pattern.findall(content)
            for match in matches:
                if match not in DAYZ_KEYWORDS and len(match) <= 6:
                    obfuscated_vars.add(match)

        if obfuscated_vars and len(obfuscated_vars) > 5:
            # Muitas variáveis curtas/aleatórias = provavelmente ofuscado
            var_list = ', '.join(sorted(obfuscated_vars)[:20])
            header = (
                f"// [PBO Deobfuscator] Variáveis possivelmente ofuscadas detectadas:\n"
                f"// {var_list}\n"
                f"// Total: {len(obfuscated_vars)} variáveis suspeitas\n\n"
            )
            content = header + content
            result.changes_made.append(
                f"Detectadas {len(obfuscated_vars)} variáveis possivelmente ofuscadas"
            )

        return content

    def _is_script(self, filename: str) -> bool:
        """Verifica se o arquivo é um script."""
        lower = filename.lower()
        return (
            lower.endswith('.c') or
            lower.endswith('.sqf') or
            lower.endswith('.sqs') or
            lower.endswith('.fsm') or
            lower.endswith('.hpp') or
            lower.endswith('.cpp')
        )

    def get_report(self) -> str:
        """Gera relatório da desobfuscação."""
        lines = [
            "=" * 60,
            "  PBO DEOBFUSCATOR - RELATÓRIO",
            "=" * 60,
            f"\nArquivo: {self.pbo.filepath}",
            f"Prefix: {self.pbo.prefix or '(nenhum)'}",
            f"Total de arquivos: {self.pbo.total_files}",
            f"Scripts analisados: {len(self.results)}",
            "",
        ]

        obfuscated_count = sum(1 for r in self.results if r.was_obfuscated)
        lines.append(f"Arquivos obfuscados detectados: {obfuscated_count}")
        lines.append("")

        if obfuscated_count > 0:
            lines.append("-" * 40)
            lines.append("DETALHES:")
            lines.append("-" * 40)

            for result in self.results:
                if result.was_obfuscated:
                    lines.append(f"\n  📄 {result.original_filename}")
                    if result.redirect_target:
                        lines.append(f"     ↳ Redirect: {result.redirect_target}")
                    for change in result.changes_made:
                        lines.append(f"     • {change}")

        lines.append("\n" + "=" * 60)
        return '\n'.join(lines)
