#pragma once
#include "common.hpp"
#include "widgets.hpp"

namespace cses {

struct WidgetProvider {
	virtual ~WidgetProvider() {}
	virtual ws::base_widget& getWidget() = 0;
	virtual void readWidget() = 0;
};

template<class T, class W>
struct BasicWidgetProvider: WidgetProvider {
	ws::base_widget& getWidget() override {
		widget.value(ref);
		return widget;
	}
	void readWidget() override {
		ref = widget.value();
	}

	BasicWidgetProvider(T& v, const string& name): ref(v) {
		widget.message(name);
	}
	W widget;
	T& ref;
};

template<class T>
using NumWidgetProvider = BasicWidgetProvider<T, ws::numeric<T>>;
using TextWidgetProvider = BasicWidgetProvider<string, ws::text>;

template<class T>
struct DefaultWidgetProvider;

template<>
struct DefaultWidgetProvider<string> {typedef TextWidgetProvider type;};

#if 0
template<class T>
struct DefaultWidgetProvider<std::enable_if<std::is_arithmetic<T>::value, T>> {typedef NumWidgetProvider<T> type;};
#endif
#if 0
template<class T, std::enable_if<std::is_arithmetic<T>::value,T>::type*=nullptr>
struct DefaultWidgetProvider<T> {typedef NumWidgetProvider<T> type;};
#endif

#if 1
template<>
struct DefaultWidgetProvider<int> {typedef NumWidgetProvider<int> type;};
template<>
struct DefaultWidgetProvider<long long> {typedef NumWidgetProvider<long long> type;};
template<>
struct DefaultWidgetProvider<double> {typedef NumWidgetProvider<double> type;};
#endif

struct FormBuilder {
	FormBuilder(cppcms::form& form): form(form) {}

	template<class T>
	FormBuilder& add(T& value, string name) {
		typedef typename DefaultWidgetProvider<T>::type W;
		return add(unique_ptr<WidgetProvider>(new W(value, name)));
	}

	FormBuilder& add(unique_ptr<WidgetProvider> provider) {
		providers.push_back(move(provider));
		return *this;
	}

	FormBuilder& build() {
		form.clear();
		for(auto& provider: providers)
			form.add(provider->getWidget());
		return *this;
	}
	FormBuilder& buildSubmit(const string& msg="Submit") {
		build();
		submit.value(msg);
		form.add(submit);
		return *this;
	}
	void readForm() {
		for(auto& provider: providers)
			provider->readWidget();
	}

private:
	vector<unique_ptr<WidgetProvider>> providers;
	ws::submit submit;
	cppcms::form& form;
};

}
