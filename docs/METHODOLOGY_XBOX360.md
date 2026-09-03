# Metodologia de Recompilação Estática — Variante Xbox 360 / rexglue-sdk

> **Estado deste documento:** referência viva, variante de `METHODOLOGY.md`. As secções 0, 1 (regras gerais), 2 (estrutura macro) e 4 (template de fase) mantêm a mesma forma do documento genérico — só a secção 3 (diagnóstico) e as secções 5-7 (específicas do runtime rexglue) mudam.
>
> Nasceu do trabalho real em dois projetos que partilham o mesmo runtime: **Forza Horizon 2** (`D:\GAME RECOMP\ForzaHorizon2_Recomp`, já em fase avançada, prova viva de que a regra de matching da secção 1 funciona na prática) e **Burnout Revenge** (`D:\Burnout Revenge Recomp`, plano concreto em `BURNOUT_REVENGE_PLAN.md`).

---

## 0. Regra Absoluta — Proibido Especular

Igual ao documento genérico, sem exceções:

- Nunca apresentar como facto algo não verificado. "Não verificado" / "por confirmar" em vez de assumir.
- Toda a afirmação técnica precisa de prova concreta anexada: output real do Ghidra MCP, log real de build, disassembly real, contagem real de funções/erros do próprio rexglue-sdk.
- Hipóteses não confirmadas ficam escritas como hipóteses (ex.: o propósito exato das secções `.embsec_*` do XEX do Burnout Revenge — ver `BURNOUT_REVENGE_PLAN.md` — ainda não está confirmado nesta sessão).
- Nenhuma fase avança com "parece que compila" ou "deve estar bem". Gate objetivo ou não avança.

---

## 1. Regra de Matching 1:1 (Assembly Original vs. Recompilado)

A regra central, igual à genérica, aplicada a **cada função, vtable, comando de GPU Xenos, buffer de áudio XMA**:

> O código C++ gerado pelo rexglue tem de corresponder exatamente ao comportamento do PowerPC original do XEX. Se uma função não fizer match — tamanho errado, instrução em falta, ramo não resolvido, vtable mal descoberta — isso é um defeito a corrigir até fazer match 1:1. Nunca "suficientemente parecido".

O procedimento operacional completo (hierarquia de oráculos, ferramenta por nível, anti-padrões) está em **`MATCHING_XBOX360.md`** — ler esse documento antes de validar qualquer fase.

**Prova de que isto já funciona na prática (Forza Horizon 2, 2026-09-01):** o ficheiro `config_default.toml` do FH2 documenta, com endereço, causa raiz e evidência (xrefs, disassembly), 8 fronteiras de função erradas encontradas e corrigidas (thunks de 16 bytes cortados para 8, uma primitiva atómica cortada antes do `lwarx/stwcx`, funções cortadas a meio) mais ~20 funções só alcançáveis por vtable/`bl`/`bctr` que o scanner automático (`vtable_scanner.cpp`) não apanhou e que só se manifestaram como crash em runtime. Este é o padrão exato a repetir no Burnout Revenge.

---

## 2. Estrutura Macro do Processo — ALPHA → BETA → STABLE

### ALPHA
**Gate:** o XEX é processado pelo pipeline de codegen do rexglue sem erro fatal, o executável compila e arranca, chegando a um estado visível (mesmo com erros gráficos).

Critérios objetivos:
- [ ] `phase_validate` do rexglue sem erros de branch direto não resolvido
- [ ] Build do runtime + código gerado compila sem erros
- [ ] Processo arranca e passa o entry point sem crash
- [ ] Primeira geometria 3D no ecrã (bugs visuais aceitáveis)
- [ ] Pelo menos uma função crítica de arranque com matching 1:1 confirmado (prova de que o processo de validação funciona)

### BETA
**Gate:** loop de jogo completo (input → lógica → física → render → áudio) funcional de ponta a ponta, bugs conhecidos documentados, matching validado no caminho crítico.

Critérios objetivos:
- [ ] Pelo menos uma pista/corrida jogável do início ao fim sem crash
- [ ] Áudio e input funcionais
- [ ] Física validada contra o original (posições/velocidades, não só "parece certo")
- [ ] Lista de bugs conhecidos (gráficos/áudio/física) existe e está atualizada

### STABLE
**Gate:** sem bugs conhecidos de gráficos/áudio no conteúdo exercitado, matching validado ao longo do caminho de código exercitado, desempenho aceitável.

Critérios objetivos:
- [ ] Todos os bugs BETA resolvidos e re-validados
- [ ] Matching 1:1 confirmado para renderização e áudio, não só CPU/lógica
- [ ] Critérios de desempenho definidos com o utilizador e cumpridos

---

## 3. Ferramentas de Diagnóstico (VRAM / GPU / CPU / Áudio) — específico rexglue-sdk

Requisito transversal desde o gate ALPHA, construído sobre infraestrutura que **já existe no rexglue-sdk** (verificado por inspeção do repositório em `D:\GAME RECOMP\rexglue-sdk\src`) — não é preciso inventar do zero:

| Domínio | O que expor | Base técnica real (rexglue-sdk) |
|---|---|---|
| VRAM/GPU (Xenos) | Estado dos registos PGRAPH/Xenos, texturas resolvidas, draw calls por frame | `src/graphics/register_file.cpp`, `registers.cpp`, `xenos.cpp`, `command_processor.cpp` |
| Captura/replay de comandos GPU | Gravar e reproduzir o stream de comandos Xenos de um frame, comparar entre execuções | **Correção (2026-09-03):** `trace_dump.cpp`/`trace_writer.cpp`/`trace_reader.cpp`/`trace_player.cpp`/`trace_viewer.cpp` foram **removidos da SDK** no commit upstream `71782a3` ("refactor(gpu): remove snappy and the GPU trace capture system") — mensagem do próprio maintainer: "o sistema de trace estava parcialmente intacto, sem visualizadores funcionais. Usa o Xenia se quiseres fazer capturas de trace de GPU deste tipo." **Não assumir que estas ferramentas existem** sem confirmar primeiro (`ls src/graphics`). Alternativas reais: (a) instalar e usar o Xenia como oráculo de captura de referência (não feito nesta sessão), (b) `packet_disassembler.cpp` ainda existe e pode servir de base para uma inspeção mais simples do stream de comandos, (c) construir uma ferramenta nova só se compensar o esforço, sabendo que a própria equipa do rexglue desistiu de manter esta classe de ferramenta |
| Backend gráfico (D3D12/Vulkan) | Pipelines/PSOs ativos, cache de shaders/texturas | `src/graphics/d3d12/*` e `src/graphics/vulkan/*` (dois backends irmãos já implementados — `pipeline_cache.cpp`, `texture_cache.cpp`, `render_target_cache.cpp` em cada um) |
| CPU (Xenon PPC) | Registos, call-stack, contadores de performance por função recompilada | `src/core/perf/` (a inventariar em detalhe na Fase 8), instrumentação do código gerado |
| Áudio (XMA) | Estado do decoder XMA, vozes ativas, buffers | `src/audio/xma_decoder.cpp`, `xma_register_file.cpp`, `audio_system.cpp`, `audio_driver.cpp` |
| Kernel/threads | Estado das threads Xenon emuladas (3 núcleos/6 threads de hardware no Xenon real — a confirmar contra `src/kernel/kernel_init.cpp`, não assumido) | `src/kernel/xboxkrnl/`, `kernel_init.cpp` |

**Regra:** um painel só está "feito" quando reflete valores reais lidos do processo (nunca mockados) e foi validado contra um caso de teste conhecido — mesma regra de matching da secção 1, aplicada à própria ferramenta.

---

### 3.1 Nota de build confirmada (Fase 0, qualquer jogo rexglue novo)

Ao configurar um projeto novo contra `D:\GAME RECOMP\rexglue-sdk` com Clang 22.1.8 (`C:\Program Files\LLVM\bin`), uma configuração "ingénua" (só `REXSDK_DIR` + compilador) falha a compilar `src/core/memory.cpp` do SDK com erro real:

```
error: always_inline function '_mm_shuffle_epi8' requires target feature 'ssse3',
but would be inlined into function 'copy_and_swap_16_aligned' that is compiled
without support for 'ssse3'
```

**Causa raiz confirmada (não é bug do rexglue-sdk):** falta a flag `-march=x86-64-v2` (que implica SSE3/SSSE3/SSE4.1/SSE4.2/POPCNT) no `CMAKE_CXX_FLAGS`/`CMAKE_C_FLAGS`. Confirmado por inspeção real do `CMakeCache.txt` do Forza Horizon 2 (build que já funciona): `CMAKE_CXX_FLAGS:STRING=-march=x86-64-v2`. Esta flag não vem de `CMakePresets.json` (que é genérico) — tem de ser passada explicitamente no comando de configure.

**Comando de configure confirmado a funcionar** (usar para qualquer jogo novo neste runtime):
```
cmake --preset win-amd64-release ^
  -DREXSDK_DIR="D:/GAME RECOMP/rexglue-sdk" ^
  -DCMAKE_C_COMPILER="C:/Program Files/LLVM/bin/clang.exe" ^
  -DCMAKE_CXX_COMPILER="C:/Program Files/LLVM/bin/clang++.exe" ^
  -DCMAKE_C_FLAGS="-march=x86-64-v2" ^
  -DCMAKE_CXX_FLAGS="-march=x86-64-v2"
```

---

## 4. Template de Fase (reutilizável)

Igual ao documento genérico:

```
### Fase N — <nome>

**Objetivo:** <uma frase, sem ambiguidade>

**Ações:**
- <passo concreto>

**Gate de validação (objetivo, binário):**
- [ ] <critério verificável>

**Matching a validar nesta fase:**
- <o que vai ser comparado original vs. recompilado>

**Evidência anexada quando a fase fechar:**
- <logs, contagens, screenshots, capturas de registo Ghidra/rexglue>
```

---

## 5. Evolução do Runtime Partilhado (rexglue-sdk)

Regra explícita pedida pelo utilizador: o rexglue-sdk **não é uma dependência estática** — é um runtime que evolui com o trabalho feito em cada jogo, beneficiando todos os outros.

1. **Nunca editar `generated/*.cpp`** (equivalente ao `runner/*.cpp` de outros kits) — é substituído a cada recompilação. Bugs do jogo corrigem-se via `config_default.toml`/`vtables.toml` (overrides); bugs do próprio SDK (recompilador, runtime, backend gráfico/áudio) corrigem-se no SDK.
2. Correções ao SDK vivem numa cópia patched — o padrão já em uso em `D:\GAME RECOMP` é exatamente este (`rexglue-patched`, `rexglue-patched-v2`: cada iteração de correções sobre o SDK oficial). Continuar esse padrão para o Burnout Revenge: se um bug do SDK for encontrado e corrigido a partir deste projeto, aplica-se à cópia patched partilhada, não só localmente.
3. Cada correção ao SDK fica documentada (ficheiro alterado, causa raiz real, jogo que a motivou, se foi ou não proposta upstream ao repositório oficial `rexglue/rexglue-sdk`) — nunca aplicada silenciosamente.
4. Antes de "reinventar" uma funcionalidade no jogo, verificar se já existe equivalente no SDK (ex.: os backends d3d12/vulkan, os decoders XMA, os scanners de vtable) — o objetivo é que o SDK fique mais completo, não que cada jogo acumule hacks locais que deviam ser genéricos.

---

## 6. Direção Futura — Renderer Nativo (pós-STABLE, não prometido)

Objetivo de longo prazo do utilizador: substituir a camada de tradução Xenos→D3D (atualmente D3D12 ou Vulkan, ambos emulando o pipeline gráfico do Xenos) por um renderizador nativo dedicado, visando **desempenho muito superior (ordem de 5x+)** e suporte a hardware mais antigo, sem depender de uma "camada extra" DirectX 9-12.

Isto é uma direção de engenharia plausível, não especulação vazia, porque é **verificável na própria arquitetura do rexglue-sdk**: `src/graphics/command_processor.cpp` já separa a interpretação do stream de comandos Xenos do backend que o consome — d3d12 e vulkan são módulos irmãos, substituíveis (`src/graphics/d3d12/command_processor.cpp` vs. `src/graphics/vulkan/command_processor.cpp` implementam a mesma interface para backends diferentes). Um terceiro backend "nativo" (mais leve, sem replicar todas as excentricidades do Xenos que o jogo concreto nunca usa) é arquiteturalmente possível dentro deste desenho.

**Isto só se aborda depois de STABLE**, com dados reais:
1. Perfilar o overhead real da emulação Xenos nos backends existentes, já a correr em STABLE (não estimado).
2. Identificar quais custos são da tradução Xenos→D3D/Vulkan vs. custos do próprio conteúdo do jogo.
3. Só então decidir, com números reais, se compensa construir um backend nativo — e para que hardware-alvo concreto.

Sem gate definido agora — fica para quando houver dados de perfilamento reais.

---

## 7. Como Reutilizar Este Kit Para Outro Jogo Xbox 360/rexglue

1. Copiar `METHODOLOGY_XBOX360.md`, `MATCHING_XBOX360.md`, `XBOX360_HARDWARE_REFERENCES.md`, `XBOX360_PROJECT_STATE_TEMPLATE.md` tal e qual para `docs/` do novo projeto.
2. Criar um `<JOGO>_PLAN.md` novo seguindo o template da secção 4, com fases concretas baseadas em factos reais desse XEX (contagem de funções do Ghidra, entry point, imports, módulos/facades) — nunca copiar as fases do Burnout Revenge sem verificar que fazem sentido para o binário novo.
3. Manter o `PROJECT_STATE.md` vivo, atualizado a cada sessão.
4. Qualquer melhoria ao rexglue-sdk feita a partir do novo jogo segue a secção 5 (partilhada, documentada, não local).

---

## Histórico de revisões
- 2026-09-03: Criado a pedido explícito do utilizador, a partir do trabalho real de planeamento do Burnout Revenge (`BurnoutRevenge_default.xex` já carregado no Ghidra MCP) e da inspeção do projeto irmão Forza Horizon 2 (prova viva da regra de matching em `config_default.toml`/`vtables.toml`).
