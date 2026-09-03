# Referências de Hardware Xbox 360 — Checklist por Subsistema

> **Nota de reconciliação (2026-09-03):** existe já um documento mais detalhado e com factos de hardware citados, escrito numa sessão anterior sobre o FH2: `D:\GAME RECOMP\docs\xbox360-static-recomp-metodologia.md` (backup de planeamento em `C:\Users\tropp\.claude\plans\xbox360-static-recomp-metodologia.md`, não alterar). Este ficheiro não duplica esse trabalho — cita os mesmos factos verificados e foca-se na aplicação por subsistema/ferramenta. Ver também memória [[xbox360-recomp-metodologia-fh2-existente]].
>
> Regra: para cada subsistema, a **fonte de verdade primária é o próprio código-fonte do rexglue-sdk** (`D:\GAME RECOMP\rexglue-sdk`), porque é já uma reimplementação funcional e ativamente mantida do hardware real. Os factos de hardware real (consola física) abaixo são citados de fontes públicas estabelecidas — servem de contexto/background, nunca substituem verificar o comportamento real no código do SDK ou no disassembly do Ghidra.

## Factos de hardware real, citados (Xbox 360 física)

- **CPU "Xenon":** 3 núcleos PowerPC de 64 bits a 3.2 GHz, 2 threads de hardware (SMT) por núcleo → 6 threads simultâneas. Cada núcleo tem uma unidade **VMX128** (128 registos vetoriais de 128 bits por thread). Cache L2 partilhada de 1 MB a 1.6 GHz, bus de 256 bits (~51.2 GB/s). Pico teórico: 115.2 GFLOPS. Fontes: [Xbox 360 technical specifications — HandWiki](https://handwiki.org/wiki/Engineering:Xbox_360_technical_specifications), [Xenon (processor) — HandWiki](https://handwiki.org/wiki/Engineering:Xenon_(processor)), [XenonWiki](https://www.xenonwiki.com/Xbox_360_Technical_Specifications).
- **GPU "Xenos":** duas dies de silício a 90nm, 500 MHz — a GPU (TSMC) + uma daughter-die de **10 MB eDRAM** (NEC) dedicada a render targets/Z-buffer. 48 unidades de shader unificado. Memória gráfica partilhada com a RAM do sistema (UMA, sem VRAM dedicada). Mesmas fontes acima.
- **Memória:** 512 MB RAM GDDR3 unificada CPU+GPU, espaço de endereçamento PowerPC de 32 bits.
- **Áudio — XMA/XMA2:** codec proprietário Microsoft baseado em WMA Pro, decodificado por DSP dedicado no hardware real.
- **Formato executável — XEX2:** assinatura ASCII "XEX2" no offset 0, cabeçalho com diretório de secções opcionais, imagem PE PowerPC habitualmente comprimida (LZX) e cifrada (AES-128-CBC). Fonte: [Free60 Wiki — XEX](https://free60.org/System-Software/Formats/XEX/).
- **Implicação direta (documentada no ficheiro-fonte acima, repetida aqui por relevância):** PowerPC é big-endian, x86-64 é little-endian — qualquer leitura/escrita de memória guest sem `__builtin_bswap32/64` correto é uma fonte silenciosa de não-matching. Já confirmámos este padrão em uso correto nos hooks reais do FH2 (`forzahorizon2_hooks.cpp`).

---

## CPU — Xenon (PowerPC tri-core, extensões VMX128)

- **Fonte primária (local, confirmada presente):** `D:\GAME RECOMP\rexglue-sdk\docs\ppc\core_instructions.pdf`, `altivec_instructions.pdf`, `vmx128.txt` — referência de instruções PPC/Altivec/VMX128 usada pelo próprio recompilador (`src/codegen/ppc/`).
- **Fonte primária (código):** `src/codegen/ppc/` (decodificação de instruções), `src/codegen/instruction_dispatch.cpp`, `src/core/math_gcc.cpp`/`math_msvc.cpp` (implementação das operações vetoriais no lado x86 recompilado).
- **Confirmado no Ghidra (Burnout Revenge):** linguagem `PowerPC:BE:64:A2ALT-32addr` / `PowerPC/big/32/PowerISA-Altivec-64-32addr` — confirma extensão Altivec presente no binário.
- **Threading:** o Xenon real tem 3 núcleos físicos, 2 threads de hardware por núcleo (6 threads simultâneas). **Não confirmado ainda** como o rexglue-sdk modela isto para um jogo concreto — verificar `src/kernel/kernel_init.cpp` e `src/core/threading*.cpp` na Fase 0/4, não assumir.
- **Invariante testável:** uma função recompilada, para os mesmos inputs de registo, produz o mesmo resultado que a disassembly do Ghidra implica — validado função a função (ver `MATCHING_XBOX360.md`).

## GPU — Xenos

- **Fonte primária (código):** `src/graphics/xenos.cpp`, `registers.cpp`, `register_file.cpp`, `command_processor.cpp`, `primitive_processor.cpp`, `packet_disassembler.cpp` — implementação real do processamento de comandos Xenos usada pelos dois backends (d3d12, vulkan).
- **Backends confirmados existentes:** `src/graphics/d3d12/*` e `src/graphics/vulkan/*` — cada um implementa `command_processor`, `pipeline_cache`, `texture_cache`, `render_target_cache`, `shader`, `shared_memory` de forma paralela (mesma interface, backend diferente). Isto é a base factual da direção "renderer nativo" descrita em `METHODOLOGY_XBOX360.md` secção 6.
- **Ferramentas de trace/replay já existentes:** `trace_dump.cpp`, `trace_writer.cpp`, `trace_reader.cpp`, `trace_player.cpp`, `trace_viewer.cpp` — usar antes de construir qualquer ferramenta de diagnóstico nova.
- **Contexto público conhecido (background, não verificado neste projeto):** o Xenos é uma GPU de shader unificado com eDRAM dedicado para render targets/tiling — mencionado aqui só como contexto arquitetural geral; qualquer número concreto (tamanho do eDRAM, largura de banda) usado no projeto tem de vir do código do SDK ou de teste real, não desta linha.
- **Invariante testável:** o stream de comandos Xenos decodificado (`packet_disassembler`) para um frame do Burnout Revenge corresponde ao capturado via `trace_dump` — comparação a fazer a partir da Fase 5.

## Áudio — XMA

- **Fonte primária (código):** `src/audio/xma_decoder.cpp`, `xma_register_file.cpp`, `audio_system.cpp`, `audio_driver.cpp`, backends `src/audio/sdl/` e `src/audio/nop/`.
- **Invariante testável:** estado do decoder XMA (vozes ativas, buffers) plausível e não-mockado para o conteúdo sonoro do jogo em cada instante — validado a partir da Fase 6.

## Kernel / XEX / ABI

- **Fonte primária (código):** `src/kernel/xboxkrnl/`, `xam/`, `xbdm/`, `crt/`, `export_table_pre.inc`/`export_table_post.inc`, `kernel_init.cpp`.
- **Estado confirmado no Burnout Revenge:** o loader do Ghidra ("XEX Loader by Warranty Voider") não listou símbolos importados (`get_imports` devolveu vazio) apesar de existir uma secção `.idata` de 0x412 bytes. **Não interpretar isto como "sem imports"** — é mais provável que o loader genérico do Ghidra não resolva a tabela de import específica do formato XEX da mesma forma que o parser dedicado do rexglue-sdk (que tem de resolver isto para gerar os shims de kernel). Confirmar via o parser do próprio rexglue na Fase 1, não assumir.
- **Segmentos confirmados (Ghidra, `get_segments`):** `.rdata`, `.pdata`, `.text` (0x537d70 bytes ≈ 5.46 MB), 6× `.embsec_*` (tamanhos entre 0xcb8 e 0x8080 bytes), `.data` (0x7680b0 bytes ≈ 7.7 MB), `.XEXID`, `.edata`, `.idata`, `.XBLD`, `.reloc`. **Hipótese não confirmada:** as secções `.embsec_*` podem corresponder a blocos do XEX processados com compressão/encriptação por secção (comportamento geral conhecido do formato XEX) — a confirmar na Fase 1/2 via o parser do rexglue-sdk, não assumido como facto.

## Memória

- **Confirmado (Ghidra):** espaço de endereços do XEX vai de `0x82000600` a `0x82dc2e57` (imagem ≈ 13.7 MB mapeada).
- O mapeamento completo de memória do Xenon real (512 MB unificados GDDR3, partilhados CPU/GPU) é contexto público conhecido, não verificado neste projeto — qualquer decisão de layout de memória do runtime recompilado segue o que `src/core/memory*.cpp` do rexglue-sdk já implementa, não uma suposição nova.

---

## Como usar este documento

Cada subsistema acima tem: (1) onde está a implementação real de referência no rexglue-sdk, (2) o que já está confirmado por inspeção real nesta sessão, (3) o que fica marcado como hipótese/por confirmar. Antes de implementar ou depurar qualquer parte do Burnout Revenge relativa a um destes subsistemas, ler o ficheiro-fonte correspondente do SDK primeiro — não assumir comportamento de hardware a partir de memória geral.

## Histórico de revisões
- 2026-09-03: Criado a partir da inspeção real do rexglue-sdk (`D:\GAME RECOMP\rexglue-sdk`) e do XEX do Burnout Revenge já carregado no Ghidra MCP.
