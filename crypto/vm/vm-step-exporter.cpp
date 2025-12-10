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
  auto boc = std_boc_serialize(std::move(cell), 0).move_as_ok();
  return td::hex_encode(boc.as_slice());
}

template <class T>
std::string object_to_boc(T obj) {
  CellBuilder builder;
  obj->serialize(builder);
  return cell_to_boc_hex(builder.finalize_novm());
}

class VmStep {
public:
  string stack_boc;
  unsigned cursor_position;

  string c0;
  string c1;
  string c2;
  string c3;
  string c4;
  string c5;
  vector<string> c7;

  explicit VmStep(VmState* state) {
    cursor_position = state->code->cur_pos();

    // Temporary nullify vm context so builders don't consume gas
    VmStateInterface::Guard vm(nullptr);

    stack_boc = object_to_boc(state->stack);

    c0 = object_to_boc(state->cr.get_c(0));
    c1 = object_to_boc(state->cr.get_c(1));
    c2 = object_to_boc(state->cr.get_c(2));
    c3 = object_to_boc(state->cr.get_c(3));

    c4 = cell_to_boc_hex(state->cr.get_d(4));
    c5 = cell_to_boc_hex(state->cr.get_d(5));

    for (StackEntry entry : *state->get_c7()) {
      c7.push_back(object_to_boc(&entry));
    }
  }

  string to_json() {
    json j;

    j["stack"] = stack_boc;
    j["cursor_position"] = cursor_position;

    j["code_registers"]["c0"] = c0;
    j["code_registers"]["c1"] = c1;
    j["code_registers"]["c2"] = c2;
    j["code_registers"]["c3"] = c3;

    j["data_registers"]["c4"] = c4;
    j["data_registers"]["c5"] = c5;

    j["c7"] = c7;

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