#pragma once
#include "common.hpp"
#include "model.hpp"
#include <cppcms/view.h>
#include <cppcms/form.h>

namespace cses {

namespace ws = cppcms::widgets;

// Widget that validates values to pass isValidUsername.
struct SetUsernameWidget: ws::text {
	virtual bool validate() override {
		if(!ws::text::validate()) return false;
		if(!isValidUsername(value())) {
			valid(false);
			error_message("Invalid username. Username must contain 1-255 characters.");
			return false;
		}
		return true;
	}
};

}
