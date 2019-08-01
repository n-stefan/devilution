#include "trace.h"

#ifdef EMSCRIPTEN

#include <emscripten.h>
EM_JS(void, trace_push, (const char* ptr), {
  var end = HEAPU8.indexOf(0, ptr);
  var text = String.fromCharCode.apply(null, HEAPU8.subarray(ptr, end));
  console.log(text);
  self.WASM_TRACE = self.WASM_TRACE || [];
  self.WASM_TRACE.push(text);
})
EM_JS(void, trace_pop, (), {
  if (self.WASM_TRACE) {
    self.WASM_TRACE.pop();
  }
})

#else

void trace_push(const char* ptr) {}
void trace_pop() {}

#endif

__Trace::Guard::Guard(const char* name, const std::string& args) {
  trace_push((name + args).c_str());
}
__Trace::Guard::~Guard() {
  trace_pop();
}
