#include "virtual_machine.h"
#include <algorithm>

namespace BehaviorTree
{

void VirtualMachine::tick(void* context) {
    index_marker = 0;
    size_t num_nodes = data_list.size();
    this_tick_running.swap(last_tick_running);
    this_tick_running.clear();
    while (index_marker < num_nodes) {
        NodeData node_data = data_list[index_marker];
        E_State state = run_action(node_data.node, context);
        assert(index_marker <= node_data.end);
        if (index_marker < node_data.end) {
            // this node should be a composite or a decorator
            assert(state == BH_RUNNING);
            running_behaviors.push_back(node_data);
        } else {
            if (state == BH_RUNNING) {
                this_tick_running.push_back(node_data.index);
            }
            run_composites(state, context);
        }
        cancel_skipped_behaviors(context);
    }
}

void VirtualMachine::run_composites(E_State state, void* context) {
    while (!running_behaviors.empty()) {
        NodeData running_node = running_behaviors.back();
        state = running_node.node->child_update(*this, context, state);
        assert(index_marker <= running_node.end);
        if (index_marker == running_node.end) {
            running_behaviors.pop_back();
            if (state == BH_RUNNING) {
                this_tick_running.push_back(running_node.index);
            }
            continue;
        } else {
            break;
        }
    }
}

E_State VirtualMachine::run_action(Node* node, void* context) {
    bool is_current_behavior_running = is_running(index_marker);
    E_State state = BH_READY;
    if (is_current_behavior_running) {
        pop_last_running_behavior();
        state = BH_RUNNING;
    } else {
        node->prepare(*this, context);
        state = BH_READY;
    }
    return node->self_update(*this, context, state);
}

void VirtualMachine::reset() {
    index_marker = 0;
    this_tick_running.clear();
    last_tick_running.clear();
    running_behaviors.clear();
}

void VirtualMachine::cancel_skipped_behaviors(void* context) {
    while (!last_tick_running.empty() && last_tick_running.back() < index_marker) {
        cancel_behavior(last_tick_running.back(), context);
        pop_last_running_behavior();
    }
}

void VirtualMachine::cancel_behavior(IndexType index, void* context) {
    Node* node = data_list[index].node;
    node->abort(*this, context);
}

void VirtualMachine::pop_last_running_behavior() {
    last_tick_running.pop_back();
}

void VirtualMachine::record_running_index(IndexType index) {
    auto iter = std::lower_bound(
            this_tick_running.rbegin(), this_tick_running.rend(), index);
    this_tick_running.insert(iter.base(), index);
}

} /* BehaviorTree */ 
