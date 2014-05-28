#pragma once
#include "common.hpp"
#include "widgets.hpp"
#include "file.hpp"
#include <cppcms/http_file.h>

namespace cses {

struct WidgetProvider {
	virtual ~WidgetProvider() {}
	WidgetProvider() {}
	WidgetProvider(const WidgetProvider&) = delete;
	WidgetProvider(const WidgetProvider&&) = delete;
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
struct FileUploadProvider: WidgetProvider {
	ws::base_widget& getWidget() override {
		return widget;
	}
	void readWidget() override {
		if (widget.set()) ref.hash = saveStreamToFile(widget.value()->data());
	}

	FileUploadProvider(T& v, const string& name): ref(v) {
		widget.message(name);
	}
	ws::file widget;
	T& ref;
};

template<class T>
struct DefaultWidgetProvider;

template<>
struct DefaultWidgetProvider<string> {typedef TextWidgetProvider type;};

template<>
struct DefaultWidgetProvider<int> {typedef NumWidgetProvider<int> type;};
template<>
struct DefaultWidgetProvider<long long> {typedef NumWidgetProvider<long long> type;};
template<>
struct DefaultWidgetProvider<double> {typedef NumWidgetProvider<double> type;};

struct FormBuilder {
	FormBuilder(cppcms::form& form): form(form) {}

	template<class T>
	FormBuilder& add(T& value, const string& name) {
		typedef typename DefaultWidgetProvider<T>::type W;
		return add(unique_ptr<WidgetProvider>(new W(value, name)));
	}

	template<class W, class T>
	FormBuilder& addWidget(T& value, const string& name) {
		typedef BasicWidgetProvider<T,W> B;
		return add(unique_ptr<WidgetProvider>(new B(value, name)));
	}

	template<class W, class T>
	FormBuilder& addProvider(T& value, const string& name) {
		return add(unique_ptr<WidgetProvider>(new W(value, name)));
	}

	FormBuilder& add(unique_ptr<WidgetProvider> provider) {
		form.add(provider->getWidget());
		providers.push_back(move(provider));
		return *this;
	}

	FormBuilder& addSubmit(const string& msg="Submit") {
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
