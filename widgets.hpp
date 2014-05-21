#pragma once
#include "common.hpp"
#include "model.hpp"
#include <cppcms/view.h>
#include <cppcms/form.h>
#include <utility>

namespace cses {

namespace ws = cppcms::widgets;

// Wrapper for Widget that checks after other validations whether checkValue
// returns an error message, and in that case sets the widget invalid with
// the returned error message.
template <typename Widget>
struct ValidatingWidget: Widget {
	virtual bool validate() override {
		if(!Widget::validate()) return false;
		optional<string> validatorRet = checkValue(Widget::value());
		if(validatorRet) {
			Widget::valid(false);
			Widget::error_message(*validatorRet);
			return false;
		}
		return true;
	}
	typedef optional<string> CheckResult;
	virtual CheckResult checkValue(const decltype(std::declval<Widget>().value())&) = 0;
	
protected:
	CheckResult ok() {
		return optional<string>();
	}
	CheckResult error(const string& message) {
		return message;
	}
};

}
