//
// Created by Vladas Zakrevskis on 10/12/2025.
//

#include "json.h"

#include "vm-step-exporter.h"

#include "boc.h"
#include "td/utils/base64.h"
#include "td/utils/misc.h"


using namespace vm;
using namespace std;
using namespace nlohmann;

std::string cell_to_boc_hex(Ref<Cell> cell) {
  auto boc = std_boc_serialize(std::move(cell), BagOfCells::Mode::WithCRC32C).move_as_ok();
  return td::hex_encode(boc.as_slice());
}

class VmStep {
public:
  string stack_boc;
  int cursor_position;

  explicit VmStep(VmState* state) {
    cursor_position = state->code->cur_pos();

    // Temporary nullify vm context so builders don't consume gas
    VmStateInterface::Guard vm(nullptr);

    CellBuilder builder;
    state->stack->serialize(builder, 0);
    auto boc = cell_to_boc_hex(builder.finalize_novm());

    stack_boc = boc;
  }

  string to_json() {
    json j;

    j["stack_boc"] = stack_boc;
    j["cursor_position"] = cursor_position;

    return j.dump(4);
  }
};


void VmStepExporter::export_step(VmState* state) {
  if (!(state->log.log_mask & vm::VmLog::DumpVerbose)) {
    return;
  }

  VM_LOG(state) << "EXECUTION_STEP_BEGIN\n" << VmStep(state).to_json();
  VM_LOG(state) << "EXECUTION_STEP_END";
}