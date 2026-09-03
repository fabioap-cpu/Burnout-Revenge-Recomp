# Regra de Matching 1:1 — Operacional (Xbox 360 / rexglue-sdk)

> Isto é a regra, não uma sugestão. Aplica-se a **cada fase** de qualquer projeto Xbox 360/rexglue (Forza Horizon 2, Burnout Revenge, ou futuro). Se uma função, vtable, comando de GPU ou amostra de áudio recompilado não corresponder ao original, é um defeito — analisa-se e corrige-se até corresponder, nunca se aceita "parecido".

---

## 1. Hierarquia de Oráculos (por ordem de confiança, do mais forte ao mais fraco)

| Nível | Oráculo | O que valida | Estado neste projeto |
|---|---|---|---|
| 1 | **Disassembly estático do Ghidra MCP** (`disassemble_at`, `get_code`, `xrefs`, `get_function_signature`) sobre o XEX original | Fronteiras de função, instruções reais, referências cruzadas (quem chama quem, vtables) | **Disponível e confirmado** — `BurnoutRevenge_default.xex` já carregado, 10597 funções detetadas automaticamente pelo Ghidra (contagem bruta, a validar caso a caso, igual ao que aconteceu no FH2) |
| 2 | **Pipeline do próprio rexglue-sdk** (`phase_scan`, `phase_discover`, `phase_gapfill`, `phase_merge`, `phase_register`, `phase_validate`, `vtable_scanner.cpp`, `sig_scanner.cpp`) | Descoberta automática de funções/vtables a partir do binário; `phase_validate` deteta branches diretos (`b`) não resolvidos | **Disponível** (rexglue-sdk compilado/build a confirmar na Fase 0) — **limitação documentada e confirmada** (comentário real em `ForzaHorizon2_Recomp/config_default.toml`): `phase_validate` só verifica saltos diretos (`b`) não resolvidos; uma função só alcançada por `bl`/`bctr` (chamada indireta/vtable) **não é apanhada estaticamente**, só falha em runtime |
| 3 | **Build + execução do binário recompilado** | Crashes, endereços inválidos, comportamento observável | Só disponível a partir da Fase 3 (primeira compilação) |
| 4 | **Xenia (emulador de referência Xbox 360)**, se instalado, como oráculo comportamental para comparar frames/áudio | Comportamento em runtime real do jogo original, sem hardware físico | **Não confirmado instalado nesta sessão** — a confirmar como ação da Fase 0/8, nunca assumido disponível |
| 5 | **Hardware Xbox 360 real** | Ground truth definitivo | **Não disponível/não aplicável** — nenhuma consola dev-kit ou ferramenta de acesso a hardware real foi mencionada ou confirmada; este nível fica fora de alcance para este projeto |

**Regra:** nunca inventar um nível de oráculo que não esteja confirmado disponível. Se só houver Nível 1-3, o matching é feito com esses — documentado como tal, não escondido.

---

## 2. Matching Obrigatório por Fase

| Fase | O que tem de fazer match 1:1 | Ferramenta |
|---|---|---|
| Scan/Discover (rexglue) | Nº de funções/vtables descobertas automaticamente vs. o que o Ghidra vê independentemente | Comparar contagem `get_functions` (Ghidra) vs. relatório do scanner do rexglue |
| Correção de fronteiras (config_default.toml) | Tamanho e instrução final de cada função corrigida manualmente | `disassemble_at` até à instrução `blr`/`b` real; documentar endereço + causa raiz (mesmo formato dos comentários já usados no FH2) |
| Vtables (vtables.toml) | Cada entrada de vtable resolvida manualmente tem de ter xref real confirmada | `xrefs` no Ghidra a apontar para o slot de vtable + para a função alvo |
| Boot/CRT (Fase 4) | Sequência de inicialização até ao entry point (`0x825B0500` no Burnout Revenge) sem desvio | Comparar call-graph estático do Ghidra a partir do entry point vs. ordem real de chamadas observada em runtime |
| Renderização (Fase 5) | Stream de comandos Xenos decodificado (`packet_disassembler.cpp`) vs. captura via `trace_dump`/`trace_writer` | Trace capturado do frame vs. replay via `trace_player`/`trace_viewer` |
| Áudio (Fase 6) | Estado do decoder XMA (vozes, buffers) plausível para o conteúdo a tocar | `xma_register_file.cpp` — inspeção de estado real, não mockado |
| Física/gameplay (Fase 7, BETA) | Posições/velocidades de entidades, não só "parece certo visualmente" | Instrumentação a construir (Fase 8), comparar contra comportamento esperado documentado |

---

## 3. Procedimento Quando NÃO Há Match

1. **Não ignorar, não silenciar, não "aceitar por agora".**
2. Isolar a camada: é o **scanner/lifter do rexglue** que errou (bug do SDK — ver secção 5 do `METHODOLOGY_XBOX360.md`), é uma **fronteira de função mal calculada** (corrigir em `config_default.toml`), é uma **vtable não descoberta** (corrigir em `vtables.toml`), ou é o **runtime** que tem um subsistema incompleto (kernel/gráfico/áudio)?
3. Corrigir na camada certa — nunca em `generated/*.cpp` (é regenerado a cada build).
4. Re-verificar o match depois da correção via o mesmo oráculo que detetou o problema.
5. Documentar o caso (endereço, causa raiz real, evidência) no `PROJECT_STATE.md` — mesmo se já corrigido, o padrão fica registado. Ver os comentários reais do FH2 em `config_default.toml` como exemplo de formato (ex.: thunk de 16 bytes cortado para 8 bytes por scan automático, corrigido depois de confirmado via disassembly + xrefs).

---

## 4. Anti-Padrões (não fazer)

- **Aceitar uma função só porque compila.** Compilar não prova matching — só prova que o C++ gerado é sintaticamente válido.
- **Marcar `phase_validate` limpo como "matching confirmado".** Só cobre branches diretos; chamadas indiretas (vtable, `bctr`) exigem verificação manual (ver secção 1, Nível 2).
- **Copiar overrides de `config_default.toml`/`vtables.toml` de outro jogo (ex. FH2) sem re-verificar no Ghidra deste XEX.** Cada binário tem os seus próprios thunks/vtables — o padrão de investigação é reutilizável, os endereços não são.
- **Avançar de fase sem o gate objetivo documentado em evidência real** (ver `METHODOLOGY_XBOX360.md` secção 2).
- **Assumir Xenia ou hardware real disponível como oráculo sem confirmar.**

---

## Histórico de revisões
- 2026-09-03: Criado a pedido explícito do utilizador, operacionalizando a regra de matching 1:1 pedida para o Burnout Revenge (e reutilizável para qualquer jogo Xbox 360/rexglue).
