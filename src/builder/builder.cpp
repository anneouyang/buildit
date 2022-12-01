#include "builder/builder.h"
#include "builder/builder_context.h"
#include "util/tracer.h"
#include "builder/dyn_var.h"

namespace builder {

template <>
std::vector<block::type::Ptr> extract_type_vector_dyn<>(void) {
	std::vector<block::type::Ptr> empty_vector;
	return empty_vector;
}

dyn_var_consume::dyn_var_consume(const var& a) {
	block_var = a.block_var;
}
dyn_var_consume::dyn_var_consume(const dyn_var_consume& a) {
	block_var = a.block_var;
}

builder builder::sentinel_builder;
void create_return_stmt(const builder &a) {
	assert(builder_context::current_builder_context != nullptr);
	builder_context::current_builder_context->remove_node_from_sequence(
	    a.block_expr);
	assert(builder_context::current_builder_context->current_block_stmt !=
	       nullptr);
	builder_context::current_builder_context->commit_uncommitted();

	if (builder_context::current_builder_context->bool_vector.size() > 0)
		return;
	block::return_stmt::Ptr ret_stmt =
	    std::make_shared<block::return_stmt>();
	//ret_stmt->static_offset = a.block_expr->static_offset;
	// Finding conflicts between return statements is somehow really hard. 
	// So treat each return statement as different. This is okay, because a 
	// jump is as bad a return. Also no performance issues
	ret_stmt->static_offset = tracer::get_unique_tag();
	ret_stmt->return_val = a.block_expr;
	builder_context::current_builder_context->add_stmt_to_current_block(
	    ret_stmt);
}
builder::builder(const var &a) {
	assert(builder_context::current_builder_context != nullptr);
	block_expr = nullptr;
	if (builder_context::current_builder_context->bool_vector.size() > 0)
		return;
	
	tracer::tag offset = get_offset_in_function();

	if (a.current_state == var::member_var) {
		assert(a.parent_var != nullptr);
		builder parent_expr_builder = (builder)(*a.parent_var);		
		
		block::member_access_expr::Ptr member = std::make_shared<block::member_access_expr>();
		member->parent_expr = parent_expr_builder.block_expr;
		builder_context::current_builder_context->remove_node_from_sequence(member->parent_expr);
		member->member_name = a.var_name;
		builder_context::current_builder_context->add_node_to_sequence(member);
		
		block_expr = member;
	} else if (a.current_state == var::compound_expr) {
		assert(a.encompassing_expr != nullptr);
		block_expr = a.encompassing_expr;
		// For now don't remove this expr from the uncommitted list
		// It should be removed when it is used
	} else if (a.current_state == var::standalone_var) {
		assert(a.block_var != nullptr);
		if (a.block_var->preferred_name == "")
                        a.block_var->preferred_name = util::find_variable_name((void*)((dyn_var<int>&)a).addr());
		block::var_expr::Ptr var_expr = std::make_shared<block::var_expr>();
		var_expr->static_offset = offset;

		var_expr->var1 = a.block_var;
		builder_context::current_builder_context->add_node_to_sequence(
		    var_expr);

		block_expr = var_expr;
	}
}

} // namespace builder
