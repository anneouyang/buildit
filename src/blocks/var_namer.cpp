#include "blocks/var_namer.h"

namespace block {
static decl_stmt::Ptr find_hoisted_decl(std::vector<stmt::Ptr>& hstmts, tracer::tag &so) {
	for (auto a: hstmts) {
		auto b = to<decl_stmt>(a);
		if (b->decl_var->static_offset == so) {
			return b;
		}
	}
	return nullptr;
}

static bool is_single_decl(block::Ptr ast, tracer::tag &so) {
	decl_counter counter;
	counter.to_find = so;
	
	ast->accept(&counter);
	return (counter.find_count == 1);
}

void decl_counter::visit(decl_stmt::Ptr a) {
	if (a->decl_var->static_offset == to_find) {
		find_count++;
	}
}
void var_namer::visit(stmt_block::Ptr a) {
	std::vector<stmt::Ptr> new_stmts;
	for (auto stmt: a->stmts) {
		if (isa<decl_stmt>(stmt)) {
			auto b = to<decl_stmt>(stmt);
			tracer::tag &so = b->decl_var->static_offset;
			decl_stmt::Ptr prev_decl = find_hoisted_decl(hoisted_decls, so);
			int detach_init = false;
			if (!prev_decl) {	
				var_counter++;
				var_replacer replacer;
				replacer.to_replace = b->decl_var;
				replacer.offset_to_replace = b->decl_var->static_offset;
				ast->accept(&replacer);
				
				if (b->decl_var->preferred_name != "") {
					// b->decl_var->var_name = b->decl_var->preferred_name + "_" + std::to_string(var_counter);
					b->decl_var->var_name = b->decl_var->preferred_name;
				} else {
					b->decl_var->var_name = "var" + std::to_string(var_counter);
				}
				// We only want to hoist those decls that do occur multiple times
				if (is_single_decl(ast, so)) {
					new_stmts.push_back(stmt);
				} else {
					hoisted_decls.push_back(b);
					detach_init = true;
				}	
			} else {
				b->decl_var = prev_decl->decl_var;
				detach_init = true;
			}
			if(detach_init && b->init_expr != nullptr && !isa<initializer_list_expr>(b->init_expr)) {
				// If the var_decl has an initialization, insert an assignment instead
				// This could have problems with C++ assignments vs copy constructors
				// This can be fixed later
				assign_expr::Ptr assign = std::make_shared<assign_expr>();
				assign->static_offset = b->static_offset;
				
				var_expr::Ptr vexpr = std::make_shared<var_expr>();
				vexpr->static_offset = b->static_offset;
				vexpr->var1 = b->decl_var;
				
				assign->var1 = vexpr;
				assign->expr1 = b->init_expr;
				b->init_expr = nullptr;
				
				expr_stmt::Ptr estmt = std::make_shared<expr_stmt>();
				estmt->static_offset = b->static_offset;
				estmt->expr1 = assign;
				estmt->annotation = b->annotation;
				b->annotation = "";
				new_stmts.push_back(estmt);	
			}
		} else {
			stmt->accept(this);
			new_stmts.push_back(stmt);
		}
	}
	a->stmts = new_stmts;
}


void var_namer::finalize_hoists(block::Ptr a) {
	auto b = to<stmt_block>(a);
	std::vector<stmt::Ptr> new_stmts;
	for (auto stmt: hoisted_decls) {
		new_stmts.push_back(stmt);	
	}
	for (auto stmt: b->stmts) {
		new_stmts.push_back(stmt);
	}
	b->stmts = new_stmts;
}
void var_replacer::visit(var_expr::Ptr a) {
	if (a->var1->static_offset == offset_to_replace) {
		if (to_replace->preferred_name == "" && a->var1->preferred_name != "")
			to_replace->preferred_name = a->var1->preferred_name;
		a->var1 = to_replace;
	}
}
} // namespace block
