#ifndef BUILDER_DYN_VAR_H
#define BUILDER_DYN_VAR_H

#include "builder/builder.h"
#include "util/var_finder.h"
namespace builder {

class var {
public:

	enum var_state {
		standalone_var,
		member_var,
		compound_expr,			
	};
	
	var_state current_state = standalone_var;

	// Optional var name
	std::string var_name;
	block::var::Ptr block_var;
	block::decl_stmt::Ptr block_decl_stmt;

        // Feature to implement members
        var *parent_var;

	// Feature to implement vars as complex expressions
	// This is require when casting a compound_expr to a 
	// type derived from dyn_var, mainly for using members
	// Avoid using this unless really required
	block::expr::Ptr encompassing_expr;
	
	static block::type::Ptr create_block_type(void) {
		// Cannot create block type for abstract class
		assert(false);
	}

	var() = default;

	explicit operator bool();

	// This is for enabling dynamic inheritance
	virtual ~var() = default;


};

// Struct to initialize a dyn_var as member;
struct as_member_of {
	var *parent_var;
	std::string member_name; 
	as_member_of(var* p, std::string n): parent_var(p), member_name(n) {};	
};
// Struct to initialize a dyn_var as a compound expr
struct as_compound_expr {
	block::expr::Ptr encompassing_expr;
	as_compound_expr(const builder &b): encompassing_expr(b.block_expr) {}	
};
using cast = as_compound_expr;

struct as_global {
	std::string name;
	as_global(const std::string &n): name(n) {}
};

template<typename T>
class dyn_var_impl: public var{
public:
	
	typedef builder BT;
	typedef dyn_var_impl<T> my_type;
	// These are required for overloads
	typedef BT associated_BT;
	typedef T stored_type;
		
	template <typename... types>
	BT operator()(const types &... args) {
		return ((BT) * this)(args...);
	}

	
	// These three need to be defined inside the class, cannot be defined globally
	BT operator=(const var &a) { 
		return (BT) * this = a; 
	}

	BT operator[] (const BT &a) {
		return ((BT) *this)[a];
	}
	BT operator* (void) {
		return ((BT) *this)[0];
	}
	BT operator=(const BT &a) {
		return (BT)*this = a;
	}

	BT operator=(const dyn_var_impl<T> &a) {
		return (BT) * this = a;
	}

	template <typename TO>
	BT operator=(const dyn_var_impl<TO> &a) {
		return (BT) * this = a;
	}

	BT operator=(const int &a) { 
		return operator=((BT)a); 
	}

	BT operator=(const double &a) { 
		return operator=((BT)a); 
	}

	BT operator=(const std::string &s) {
		return operator=((BT)s);
	}
	BT operator=(char* s) {
		return operator=((BT)s);
	}
	BT operator=(const char* s) {
		return operator=((BT)s);
	}

	template <typename Ts>
	BT operator=(const static_var<Ts> &a) {
		return operator=((BT)a);
	}

	BT operator!() { return !(BT) * this; }
	operator bool() { return (bool)(BT) * this; }

	static block::type::Ptr create_block_type(void) { 
		return type_extractor<T>::extract_type(); 
	}

	void create_dyn_var(bool create_without_context = false) {
		if (create_without_context) {
			block::var::Ptr dyn_var = std::make_shared<block::var>();
			dyn_var->var_type = create_block_type();
			block_var = dyn_var;
			// Don't try to obtain preferred names for objects created without context
			//dyn_var->preferred_name = util::find_variable_name(this);
			return;
		}
		assert(builder_context::current_builder_context != nullptr);
		assert(builder_context::current_builder_context->current_block_stmt != nullptr);
		builder_context::current_builder_context->commit_uncommitted();
		block::var::Ptr dyn_var = std::make_shared<block::var>();
		dyn_var->var_type = create_block_type();
		dyn_var->preferred_name = util::find_variable_name(this);
		block_var = dyn_var;
		tracer::tag offset = get_offset_in_function();
		dyn_var->static_offset = offset;
		block_decl_stmt = nullptr;
		if (builder_context::current_builder_context->bool_vector.size() > 0)
			return;
		block::decl_stmt::Ptr decl_stmt = std::make_shared<block::decl_stmt>();
		decl_stmt->static_offset = offset;
		decl_stmt->decl_var = dyn_var;
		decl_stmt->init_expr = nullptr;
		block_decl_stmt = decl_stmt;
		builder_context::current_builder_context->add_stmt_to_current_block(decl_stmt);
	}

	dyn_var_impl() {
		create_dyn_var(false);
	}
	// Basic and other constructors
	dyn_var_impl(const as_global &v) { 
		if (builder_context::current_builder_context == nullptr) {
			create_dyn_var(true); 
			block_var->var_name = v.name;
			var_name = v.name;
		} else {
			create_dyn_var(false); 
			block_var->var_name = v.name;
			var_name = v.name;
		}
		// Now that we have created the block_var, we need to leak a reference
		// So that the destructor for the block_var is never called
		auto ptr_to_leak = new std::shared_ptr<block::block>();
		*ptr_to_leak = block_var;
	}
	dyn_var_impl(const dyn_var_sentinel_type& a, std::string name = "") {
		create_dyn_var(true);
		if (name != "") {
			block_var->var_name = name;
			var_name = name;
		}	
	}
        // Constructor to initialize a dyn_var as member
        // This declaration does not produce a declaration
        dyn_var_impl(const as_member_of &a) {
		current_state = member_var;
		parent_var = a.parent_var;
		var_name = a.member_name;            
		block_var = nullptr;
		block_decl_stmt = nullptr;
	}
	// Constructor and operator = to initialize a dyn_var as a compound expr
	// This declaration also does not produce a declaration or assign stmt
	dyn_var_impl(const as_compound_expr &a) {
		current_state = compound_expr;
		parent_var = nullptr;
		block_var = nullptr;
		block_decl_stmt = nullptr;	
		encompassing_expr = a.encompassing_expr;	
	}
	void operator = (const as_compound_expr& a) {
		current_state = compound_expr;
		parent_var = nullptr;
		block_var = nullptr;
		block_decl_stmt = nullptr;	
		encompassing_expr = a.encompassing_expr;	
	}
	// A very special move constructor that is used to create exact 
	// replicas of variables
	dyn_var_impl(const dyn_var_consume &a) {
		block_var = a.block_var;
		var_name = block_var->var_name;
		block_decl_stmt = nullptr;	
		
	}

	dyn_var_impl(const my_type &a) : my_type((BT)a) {}

	
	template <typename TO>
	dyn_var_impl(const dyn_var_impl<TO> &a) : my_type((BT)a) {}
	
	template <typename TO>
	dyn_var_impl(const static_var<TO> &a) : my_type((TO)a) {}

	dyn_var_impl(const BT &a) {
		builder_context::current_builder_context->remove_node_from_sequence(a.block_expr);
		create_dyn_var();
		if (builder_context::current_builder_context->bool_vector.size() > 0)
			return;
		block_decl_stmt->init_expr = a.block_expr;
	}

	dyn_var_impl(const int &a) : my_type((BT)a) {}
	dyn_var_impl(const bool &a) : my_type((BT)a) {}
	dyn_var_impl(const double &a) : my_type((BT)a) {}
	dyn_var_impl(const float &a) : my_type((BT)a) {}
	dyn_var_impl(const std::string &a) : my_type((BT)a) {}
	dyn_var_impl(const char* s): my_type((BT)(std::string)s) {}
	dyn_var_impl(char* s): my_type((BT)(std::string)s) {}

	dyn_var_impl(const std::initializer_list<BT> &_a) {
		std::vector<BT> a(_a);

		assert(builder_context::current_builder_context != nullptr);
		for (unsigned int i = 0; i < a.size(); i++) {
			builder_context::current_builder_context->remove_node_from_sequence(a[i].block_expr);
		}
		create_dyn_var();
		if (builder_context::current_builder_context->bool_vector.size() > 0)
			return;

		tracer::tag offset = get_offset_in_function();
		block::initializer_list_expr::Ptr list_expr = std::make_shared<block::initializer_list_expr>();
		list_expr->static_offset = offset;
		for (unsigned int i = 0; i < a.size(); i++) {
			list_expr->elems.push_back(a[i].block_expr);
		}
		block_decl_stmt->init_expr = list_expr;
	}


	virtual ~dyn_var_impl() = default;


	// Assume that _impl objects will never be created
	// Thus addr can always cast the address to dyn_var<T>
	dyn_var<T>* addr(void) {
		// TODO: Consider using dynamic_cast here
		return (dyn_var<T>*)this;
	}

};

// Actual dyn_var implementation
// Split design to allow for easily extending types with specialization

template<typename T>
class dyn_var: public dyn_var_impl<T> {
public:
	typedef dyn_var_impl<T> super;
	
	using super::super;
	using super::operator=;
	
	dyn_var(): dyn_var_impl<T>() {}
	
	// Some implementations don't like implicitly declared
	// constructors so define them here
	dyn_var(const dyn_var<T> &t): dyn_var_impl<T>((builder)t){}

	// Unfortunately because we are changing the return type, 
	// the implicitly defined copy assignment will always 
	// shadow the version the parent defines
	builder operator= (const dyn_var<T> &t) {
		return *this = (builder)t;
	}
};


template <typename T>
typename std::enable_if<std::is_base_of<var, T>::value>::type create_return_stmt(const T &a) {
	create_return_stmt((typename T::associated_BT)a);	
}


}

#endif
