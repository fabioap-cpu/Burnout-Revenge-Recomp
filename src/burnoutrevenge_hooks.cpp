#include <windows.h>
#include <rex/logging.h>

namespace {

// Diagnostic-only vectored exception handler: logs the real exception code,
// faulting address, and register state before letting the process continue
// searching for a handler (never recovers anything here -- see
// ForzaHorizon2_Recomp/src/forzahorizon2_hooks.cpp's CrashFilter for the
// established pattern this is modeled on, which DOES recover guest-page
// faults once the fault class is understood; this one does not, because the
// crash class after "Creating graphics pipeline" has not been diagnosed yet
// and guessing a recovery would violate the no-especulação rule).
LONG WINAPI DiagnosticExceptionFilter(EXCEPTION_POINTERS* ep) {
  if (ep && ep->ExceptionRecord) {
    auto* rec = ep->ExceptionRecord;

    char module_name[MAX_PATH] = "<unknown>";
    HMODULE mod = nullptr;
    if (GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(rec->ExceptionAddress), &mod) &&
        mod) {
      GetModuleFileNameA(mod, module_name, sizeof(module_name));
    }
    uint64_t offset = mod ? reinterpret_cast<uint64_t>(rec->ExceptionAddress) -
                                reinterpret_cast<uint64_t>(mod)
                          : 0;

    REXKRNL_WARN(
        "[CrashDiag] exception code=0x{:08X} address=0x{:016X} module={} "
        "offset=0x{:X} flags=0x{:08X} num_params={}",
        static_cast<uint32_t>(rec->ExceptionCode),
        reinterpret_cast<uint64_t>(rec->ExceptionAddress), module_name, offset,
        rec->ExceptionFlags, rec->NumberParameters);
    for (DWORD i = 0; i < rec->NumberParameters && i < EXCEPTION_MAXIMUM_PARAMETERS; ++i) {
      REXKRNL_WARN("[CrashDiag]   param[{}]=0x{:016X}", i,
                   static_cast<uint64_t>(rec->ExceptionInformation[i]));
    }
#if defined(_M_X64)
    if (ep->ContextRecord) {
      REXKRNL_WARN(
          "[CrashDiag] rip={:016X} rax={:016X} rbx={:016X} rcx={:016X} rdx={:016X} "
          "rsp={:016X} rbp={:016X} MxCsr=0x{:08X}",
          ep->ContextRecord->Rip, ep->ContextRecord->Rax, ep->ContextRecord->Rbx,
          ep->ContextRecord->Rcx, ep->ContextRecord->Rdx, ep->ContextRecord->Rsp,
          ep->ContextRecord->Rbp, ep->ContextRecord->MxCsr);
    }
#endif
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

struct DiagnosticHandlerInit {
  DiagnosticHandlerInit() { AddVectoredExceptionHandler(1, DiagnosticExceptionFilter); }
} g_diagnosticHandlerInit;

}  // namespace
