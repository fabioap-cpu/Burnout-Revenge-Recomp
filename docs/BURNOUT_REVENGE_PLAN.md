# Plano por Fases — Recompilação Estática de Burnout Revenge (Xbox 360 → PC/DirectX)

## Contexto

Objetivo: recompilar estaticamente o binário Xbox 360 (`BurnoutRevenge_default.xex`) de **Burnout Revenge** (Criterion Games, 2005/2006) para um executável nativo Windows, com pipeline gráfica, áudio e CD/ficheiros próprios, usando o runtime [`rexglue-sdk`](https://github.com/rexglue/rexglue-sdk) (`D:\GAME RECOMP\rexglue-sdk`) já instalado, complementado por análise manual via Ghidra MCP. Tudo vive em `D:\Burnout Revenge Recomp` (atualmente vazia).

Este plano segue `D:\RECOMP_METHODOLOGY\METHODOLOGY_XBOX360.md` e `MATCHING_XBOX360.md` — regras genéricas para qualquer jogo Xbox 360/rexglue, não repetidas aqui. Cópia viva em `D:\Burnout Revenge Recomp\docs\`.

**Verificação já feita nesta sessão (factos confirmados, não especulação):**
- `BurnoutRevenge_default.xex` está carregado e ativo no Ghidra MCP. Linguagem `PowerPC:BE:64:A2ALT-32addr` (`PowerPC/big/32/PowerISA-Altivec-64-32addr`) — confirma alvo Xbox 360 com Altivec, não Xbox clássico.
- **Function Count (bruto):** 10597 funções detetadas pela auto-análise do Ghidra. Isto é uma contagem bruta do analisador genérico — o projeto irmão Forza Horizon 2 mostrou, na prática, que uma fração destas fronteiras vem errada (thunks cortados, funções só alcançáveis por vtable) e só se descobre com o processo da secção "Correção de fronteiras" abaixo. Não assumir que 10597 é o número final de funções reais.
- **Entry point:** `0x825B0500`.
- **Layout de memória:** imagem mapeada de `0x82000600` a `0x82dc2e57` (~13.7 MB). `.text` = 0x537d70 bytes (~5.46 MB código), `.data` = 0x7680b0 bytes (~7.7 MB), mais `.rdata`, `.pdata`, 6 secções `.embsec_*` (0xcb8–0x8080 bytes cada, propósito não confirmado — ver `XBOX360_HARDWARE_REFERENCES.md`), `.edata`, `.idata` (0x412 bytes), `.XEXID`, `.XBLD`, `.reloc`.
- **Imports:** `get_imports` do Ghidra devolveu vazio. Não interpretado como "sem imports do kernel" — mais provável ser uma limitação do loader genérico do Ghidra para o formato XEX; a confirmar via o parser dedicado do rexglue na Fase 1.
- **Ficheiros do jogo já descomprimidos e desencriptados** em `D:\GAME RECOMP\Burnout Revenge (Europe) (En,Es,Nl,Sv,Fi)\`, com `BurnoutRevenge_default.xex`, `default.xex` e pastas de conteúdo (`data`, `eatrax`, `fe`, `graphics`, `language`, `ovid`, `pveh`, `sound`, `tracks`). ISO de origem em `D:\GAME RECOMP\Burnout Reveng\`. **Não confirmado ainda** se `default.xex` e `BurnoutRevenge_default.xex` são o mesmo binário ou módulos distintos (facades, à semelhança do FH2 que tem `XMediaFacade_default.xex`/`SpeechFacade_default.xex`) — verificar na Fase 1.
- **rexglue-sdk é uma ferramenta real, funcional, ativamente usada**: o projeto irmão `D:\GAME RECOMP\ForzaHorizon2_Recomp` já usa este runtime em fase avançada (manifest gerado por "ReXGlue v0.10.0.1-dev.ge9d4425", sdk_version "0.9.0"), com `config_default.toml`/`vtables.toml` reais contendo correções documentadas de fronteiras de função e vtables, scripts de build (Ninja + clang/clang++ via CMakePresets) e de execução em `tools/*.ps1`.
- Existem cópias já corrigidas do rexglue-sdk em `D:\GAME RECOMP\rexglue-bin`, `rexglue-patched`, `rexglue-patched-v2` — **a confirmar na Fase 0** se são diretamente reaproveitáveis para o Burnout Revenge ou se é necessário recompilar a partir de `rexglue-sdk` fonte.

**Isto significa, com honestidade:** este é um projeto de investigação/engenharia real de meses, não semanas — mesmo com um toolkit mais maduro que o `xboxrecomp` genérico (o rexglue-sdk já tem um jogo terceiro em estado avançado, o FH2), o Burnout Revenge é um binário novo, com o seu próprio motor (RenderWare-like, a confirmar — Criterion tinha motor próprio, não confirmado se é RenderWare), as suas próprias fronteiras de função erradas e vtables por descobrir. Não há garantia de prazo. Cada fase abaixo tem um gate objetivo; não se avança sem o passar.

---

## Fases

### Fase 0 — Ambiente e reaproveitamento do toolkit
**Objetivo:** confirmar que o rexglue-sdk (fonte ou pré-compilado) está pronto para gerar código para um XEX novo, sem tocar ainda no Burnout Revenge.

**Ações:**
- Confirmar se `rexglue-bin`/`rexglue-patched-v2` contêm um CLI do rexglue utilizável diretamente, ou se é preciso `cmake --build` a partir de `rexglue-sdk` (Ninja + clang/clang++, conforme `CMakePresets.json` do FH2, reaproveitável tal e qual).
- Criar `D:\Burnout Revenge Recomp\` com layout espelhando `ForzaHorizon2_Recomp` (`CMakeLists.txt`, `CMakePresets.json`, `docs/`, `tools/`, `generated/`, `content/`).
- Confirmar no Ghidra que `default.xex` vs `BurnoutRevenge_default.xex` são o mesmo binário ou módulos distintos (ação simples: `mcp__ghidra__import_file`/`open_program` sobre `default.xex` e comparar `get_binary_info`).

**Gate de validação:**
- [x] CLI do rexglue corre e produz output real — `rexglue init` gerou o scaffold real do projeto
- [x] `D:\Burnout Revenge Recomp` criada com a estrutura base (via `rexglue init`, não escrita à mão)
- [x] Relação entre `default.xex` e `BurnoutRevenge_default.xex` confirmada — mesmo executável em dois estados de processamento (original vs. descomprimido/desencriptado pelo utilizador), não facades distintas
- [x] SDK partilhada (`D:\GAME RECOMP\rexglue-sdk`, branch `alexbeav-clean`) compila limpa contra este projeto, confirmando `REXSDK_DIR` como mecanismo correto (não `rexglue-bin`/`rexglue-patched*`, que são snapshots desatualizados)

**FASE 0 FECHADA em 2026-09-03.** Dois problemas reais encontrados e corrigidos (ver `PROJECT_STATE.md`): falta de `-march=x86-64-v2` (causava erro de compilação em `src/core/memory.cpp` do SDK, intrínseco SSSE3) e propagação de include do imgui em falta (mesma correção já usada no `CMakeLists.txt` do FH2). Confirmado por evidência real que o output da SDK é partilhado no disco com o FH2 (`rexruntime.dll` recompilado no mesmo local).

**Comando de configure confirmado:**
```
cmake --preset win-amd64-release -DREXSDK_DIR="D:/GAME RECOMP/rexglue-sdk" -DCMAKE_C_COMPILER="C:/Program Files/LLVM/bin/clang.exe" -DCMAKE_CXX_COMPILER="C:/Program Files/LLVM/bin/clang++.exe" -DCMAKE_C_FLAGS="-march=x86-64-v2" -DCMAKE_CXX_FLAGS="-march=x86-64-v2"
```

**Matching a validar:** nenhum ainda — fase de preparação.

**Evidência anexada:** `D:\Burnout Revenge Recomp\docs\fase0_build_log.txt` (log real do build), `PROJECT_STATE.md` (detalhe completo).

---

### Fase 1 — Geração do manifest e scan inicial (rexglue codegen)
**Objetivo:** primeira passagem automática do rexglue sobre o Burnout Revenge, produzindo `manifest.toml`, `config_default.toml` e `vtables.toml` base.

**Ações:**
- Correr o pipeline do rexglue (`phase_scan` → `phase_discover` → `phase_gapfill` → `phase_merge` → `phase_register` → `phase_validate`) sobre `BurnoutRevenge_default.xex`, gerando o manifest inicial (mesmo processo que produziu o `forzahorizon2_manifest.toml` real).
- Registar a contagem de funções/vtables que o scanner do rexglue descobre, e comparar com as 10597 do Ghidra — divergências são esperadas e informativas, não um erro a "resolver" já aqui.
- Investigar a tabela de imports do XEX (kernel/xam) usando o parser do próprio rexglue (não o loader genérico do Ghidra) — resolve a incerteza deixada na Fase 0/contexto sobre `get_imports` vazio.
- Investigar o propósito real das 6 secções `.embsec_*`.

**Gate de validação:**
- [ ] `manifest.toml` + `config_default.toml` + `vtables.toml` gerados sem erro fatal do pipeline
- [ ] Relatório real (não estimado) de nº de funções descobertas automaticamente vs. flags/avisos do `phase_validate`
- [ ] Tabela de imports do kernel resolvida (lista real de símbolos xboxkrnl/xam usados)
- [ ] Propósito das secções `.embsec_*` confirmado ou explicitamente marcado como "não resolvido, sem impacto conhecido até à Fase X"

**Matching a validar:** nenhum ainda (é geração automática) — mas a base para todo o matching das fases seguintes.

**Evidência anexada:** manifest/config/vtables gerados, log real do pipeline, contagens comparadas.

---

### Fase 2 — Correção de fronteiras de função e vtables (Ghidra MCP a fundo)
**Objetivo:** aplicar o processo já provado no FH2 — cada aviso do `phase_validate` e cada crash por chamada indireta investigado e corrigido individualmente, com evidência.

**Ações:**
- Para cada branch direto não resolvido reportado pelo `phase_validate`: `disassemble_at` no Ghidra, confirmar tamanho/fronteira real, corrigir em `config_default.toml` com comentário documentando causa raiz (seguir exatamente o formato usado no FH2: thunk cortado, primitiva atómica cortada, função cortada a meio, etc.).
- Para vtables: usar `vtable_scanner.cpp` como primeira passagem automática; para os casos que falharem (só descobertos por crash em runtime na Fase 3+), confirmar via `xrefs` no Ghidra e adicionar a `vtables.toml`.
- Documentar cada correção na tabela de `PROJECT_STATE.md`.

**Gate de validação:**
- [ ] Zero avisos de `phase_validate` sem explicação documentada
- [ ] Pelo menos N correções de fronteira documentadas com endereço + causa raiz + evidência (N real, reportado, não prometido antecipadamente)

**Matching a validar:** tamanho e instrução final de cada função corrigida (ver `MATCHING_XBOX360.md` secção 2).

**Evidência anexada:** tabela de correções no `PROJECT_STATE.md`, capturas de `disassemble_at`/`xrefs` por caso.

---

### Fase 3 — Primeira compilação do código recompilado
**Objetivo:** gerar o C++ completo e obter um executável que compila.

**Ações:**
- Gerar `generated/default/` a partir do manifest corrigido.
- Ligar contra as bibliotecas do rexglue-sdk (kernel, graphics, audio, core) e compilar (Ninja + clang/clang++).
- Listar (grep real) instruções/padrões não suportados pelo codegen para este binário específico.

**Gate de validação:**
- [ ] Build limpo, executável produzido
- [ ] Lista real de gaps do lifter para este XEX reportada ao utilizador (não assumida como 100% coberta)

**Matching a validar:** nenhum comportamental ainda — só compilação.

**Evidência anexada:** log de build real.

---

### Fase 4 — Boot / arranque (marco ALPHA, parte 1 de 2)
**Objetivo:** o executável arranca sem crash até passar o entry point e o CRT.

**Ações:**
- Implementar/ligar os shims de kernel (`src/kernel/xboxkrnl`, `xam`) necessários, resolvidos pela lista de imports da Fase 1.
- Configurar layout de memória e threads conforme `src/kernel/kernel_init.cpp`.

**Gate ALPHA (1/2):**
- [ ] Processo arranca, passa `0x825B0500` (entry point) e a inicialização do CRT sem crash.

**Matching a validar:** sequência de chamadas de arranque (call-graph estático do Ghidra a partir do entry point vs. execução real).

**Evidência anexada:** log de execução real até ao ponto de paragem/crash seguinte (se houver).

---

### Fase 5 — Renderização inicial (marco ALPHA, parte 2 de 2)
**Objetivo:** primeira geometria 3D do Burnout Revenge no ecrã.

**Ações:**
- Ligar `graphics_system` (escolher D3D12 ou Vulkan como primeiro backend — ambos existem no SDK) e resolver o primeiro stream de comandos Xenos do jogo.

**Gate ALPHA (2/2):**
- [ ] Primeira geometria 3D visível (texturas/efeitos errados são aceitáveis).

**Matching a validar:** stream de comandos Xenos decodificado (`packet_disassembler`) vs. captura (`trace_dump`).

**Evidência anexada:** screenshot real, trace capturado.

---

### Fase 6 — Áudio + Input
**Objetivo:** som básico audível, input responde.

**Ações:**
- Ligar `audio_system`/`xma_decoder` (backend SDL) e input.

**Gate:**
- [ ] Áudio audível, comandos de input respondem.

**Matching a validar:** estado do decoder XMA plausível para o conteúdo tocado.

**Evidência anexada:** captura de áudio real, log de eventos de input.

---

### Fase 7 — Estabilização do loop de gameplay (marco BETA)
**Objetivo:** pelo menos uma pista/corrida jogável do início ao fim.

**Ações:**
- Depurar crashes de gameplay, resolver vtables/ICALLs em falta iterativamente (mesmo ciclo: crash → identificar → corrigir → re-validar).

**Gate BETA:**
- [ ] Pelo menos uma pista jogável do início ao fim, sem crash, áudio e input funcionais
- [ ] Física validada contra o original (posições/velocidades reais, não só "parece certo")
- [ ] Lista de bugs conhecidos (gráficos/áudio/física) existe e está atualizada

**Matching a validar:** física de entidades, ver `MATCHING_XBOX360.md` tabela secção 2.

**Evidência anexada:** vídeo/gameplay real, lista de bugs documentada.

---

### Fase 8 (paralela, a partir da Fase 4) — Ferramentas de diagnóstico
**Objetivo:** painéis reais de VRAM/GPU/CPU/Áudio, construídos sobre a infraestrutura já existente no rexglue-sdk (ver `METHODOLOGY_XBOX360.md` secção 3): `register_file.cpp`, `trace_dump`/`trace_player`/`trace_viewer`, `src/core/perf`, `xma_register_file.cpp`.

**Gate:** cada painel reflete valores reais lidos do processo, validado contra um caso de teste conhecido.

**Matching a validar:** os próprios valores mostrados nos painéis, contra o estado real do processo.

**Evidência anexada:** captura de cada painel correspondendo a um estado conhecido do jogo.

---

### Fase 9 — Versão estável (marco STABLE)
**Objetivo:** sem bugs conhecidos de gráficos/áudio no conteúdo exercitado.

**Gate STABLE:**
- [ ] Todos os bugs da Fase 7 resolvidos e re-validados
- [ ] Matching 1:1 confirmado para renderização e áudio
- [ ] Critérios de desempenho definidos com o utilizador nesta fase e cumpridos

---

### Fase 10 (contínua, a partir de STABLE) — Evolução do runtime partilhado
**Objetivo:** qualquer correção ao rexglue-sdk feita a partir deste projeto beneficia outros jogos (FH2 incluído), seguindo `METHODOLOGY_XBOX360.md` secção 5.

**Gate:** cada patch ao SDK documentado (ficheiro, causa raiz, jogo motivador, proposto upstream ou não) — nunca aplicado silenciosamente.

---

### Fase 11 (investigação pós-STABLE, não prometida) — Renderer nativo
**Objetivo:** avaliar, com dados reais de perfilamento, se compensa substituir a emulação Xenos→D3D/Vulkan por um backend nativo dedicado, visando desempenho muito superior e suporte a hardware mais antigo (ver `METHODOLOGY_XBOX360.md` secção 6).

**Ações (só depois de STABLE):**
- Perfilar overhead real da emulação Xenos nos backends existentes já a correr em STABLE.
- Separar custo de tradução Xenos→backend vs. custo do conteúdo do jogo.

**Gate:** sem gate definido agora — decisão tomada com dados reais quando existirem.

---

## O que este plano não promete
- Não há garantia de o Burnout Revenge chegar a "completo" em prazo definido.
- O motor de jogo da Criterion não está confirmado como RenderWare ou proprietário — isso só se confirma na Fase 1/2.
- Multiplayer/online (se existir na versão Xbox 360) fica fora de âmbito nesta fase do plano.
- A Fase 11 (renderer nativo) é uma direção de investigação, não um compromisso de entrega.

## Verificação
Cada fase reporta ao utilizador o resultado real (build logs, contagens, screenshots, capturas de trace) antes de avançar — nunca se assume sucesso sem o artefacto correspondente, seguindo `METHODOLOGY_XBOX360.md` secção 0.

## Histórico de revisões
- 2026-09-03: Criado a partir de factos verificados no Ghidra MCP (XEX já carregado) e na inspeção real do rexglue-sdk e do projeto irmão Forza Horizon 2.
