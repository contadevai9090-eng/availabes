# PBO Deobfuscator 🔓

> **Desobfuscador de arquivos PBO para DayZ e ArmA 3** — Inspirado no [pbo.tools](https://pbo.tools/)

Ferramenta Python para abrir, analisar, extrair e desobfuscar arquivos `.pbo` protegidos por ferramentas como PBO Tools, ObfuSQF e similares.

## Funcionalidades

- **Abrir PBO** — Parse completo do formato binário PBO (compatível com DayZ e ArmA 3)
- **Listar conteúdo** — Visualize todos os arquivos dentro do PBO
- **Extrair arquivos** — Extrai todo o conteúdo (mesmo obfuscado)
- **Desobfuscar scripts** — Reverte técnicas comuns de obfuscação:
  - Include redirect (arquivo que aponta para outro via `#include`)
  - Código minificado (reformata e indenta)
  - Dead code / junk code injection
  - Strings ofuscadas (concatenação, charcode, reverse)
  - Detecção de variáveis renomeadas
- **Descompressão LZSS** — Suporta PBOs com compressão
- **Relatório detalhado** — Gera relatório com todas as operações realizadas

## Instalação

```bash
cd deobfuscator

# Instalar como pacote
pip install -e .

# Ou executar diretamente
python -m pbo_deobfuscator.cli --help
```

## Uso

### Informações do PBO

```bash
pbo-deobfuscator info meu_mod.pbo
```

Mostra: tamanho, número de arquivos, propriedades, prefix, e sinais de obfuscação detectados.

### Listar arquivos

```bash
pbo-deobfuscator list meu_mod.pbo
```

### Extrair conteúdo

```bash
pbo-deobfuscator extract meu_mod.pbo -o pasta_saida/
```

### Desobfuscar (principal)

```bash
pbo-deobfuscator deobfuscate meu_mod.pbo -o pasta_saida/
# Alias curtos:
pbo-deobfuscator deobf meu_mod.pbo
pbo-deobfuscator d meu_mod.pbo
```

Este comando:
1. Extrai todos os arquivos do PBO
2. Analisa scripts em busca de obfuscação
3. Aplica transformações de desobfuscação
4. Salva arquivos limpos + relatório

## Como funciona a desobfuscação

### Include Redirect
PBO Tools e similares substituem o conteúdo real dos scripts por um simples `#include "path/to/real/file"`, escondendo o código real em outro arquivo dentro do PBO. O desobfuscador segue esses redirects e restaura o conteúdo original.

### Código Minificado
Scripts com todo o código em uma única linha são reformatados com indentação adequada para facilitar a leitura.

### Dead Code
Código injetado que nunca executa (ex: `if(false){...}`) é detectado e removido.

### Strings Ofuscadas
Strings quebradas em caracteres individuais (`"h"+"e"+"l"+"l"+"o"`) ou codificadas em char codes são reconstruídas.

### Variáveis Ofuscadas
Variáveis com nomes aleatórios curtos (ex: `_0xA1B2`, `a1`, `bc23`) são detectadas e anotadas no código para facilitar análise manual.

## Formato PBO

O formato PBO (Packed Bank of Objects) é um container binário usado pela Bohemia Interactive:

```
┌─────────────────────────────────┐
│ Properties Header (opcional)     │  <- "Vers" + key=value pairs
├─────────────────────────────────┤
│ Entry 1 (filename + metadata)    │  <- 21+ bytes cada
│ Entry 2                          │
│ ...                              │
│ Entry N                          │
│ Empty Entry (fim do header)      │  <- filename vazio
├─────────────────────────────────┤
│ Data Block (contíguo)            │  <- dados dos arquivos
├─────────────────────────────────┤
│ SHA1 Checksum (opcional)         │  <- 1 + 20 bytes
└─────────────────────────────────┘
```

## Requisitos

- Python 3.10+
- Sem dependências externas (usa apenas stdlib)

## Uso como biblioteca Python

```python
from pbo_deobfuscator.pbo_parser import parse_pbo, extract_all
from pbo_deobfuscator.deobfuscator import PBODeobfuscator

# Parse do PBO
pbo = parse_pbo("meu_mod.pbo")

# Info
print(f"Arquivos: {pbo.total_files}")
print(f"Scripts: {len(pbo.scripts)}")
print(f"Prefix: {pbo.prefix}")

# Extrair tudo
extract_all(pbo, "output/")

# Desobfuscar
deobf = PBODeobfuscator(pbo)
results = deobf.deobfuscate_all()

# Relatório
print(deobf.get_report())

# Verificar resultado individual
for result in results:
    if result.was_obfuscated:
        print(f"{result.original_filename}: {result.changes_made}")
```

## Limitações

- **Criptografia**: PBOs criptografados (`.ebo`) não são suportados
- **Obfuscação avançada**: Técnicas como control flow flattening não são totalmente reversíveis automaticamente
- **Nomes de variáveis**: A ferramenta detecta variáveis ofuscadas mas não pode recuperar os nomes originais

## Licença

MIT — Use responsavelmente. Esta ferramenta deve ser usada apenas para análise de mods próprios ou autorizados.
