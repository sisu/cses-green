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
template<class T, class Ptr>
struct SelectProvider: WidgetProvider {
	ws::base_widget& getWidget() override {
		for(const auto& i: choises) widget.add(i.first, std::to_string(i.second->id));
		return widget;
	}
	void readWidget() override {
		optional<ID> id = stringToInteger<ID>(widget.selected_id());
		if (!id) return;
		for(const auto& i: choises) {
			if (*id == i.second->id) ref = i.second;
		}
	}

	SelectProvider(Ptr& v, const string& name, vector<pair<string,shared_ptr<T>>> choises):
		ref(v), choises(choises)
	{
		widget.message(name);
	}
	ws::select widget;
	Ptr& ref;
	vector<pair<string, shared_ptr<T>>> choises;
};

struct CheckboxProvider: WidgetProvider {
	ws::base_widget& getWidget() override {
		widget.value(ref);
		return widget;
	}
	void readWidget() override {
		ref = widget.value();
	}
	
	CheckboxProvider(bool& v, const string& name) : ref(v) {
		widget.message(name);
	}
	
	ws::checkbox widget;
	bool& ref;
};

template<class T, class...A>
unique_ptr<T> makeUnique(A&&...args) {
	return unique_ptr<T>(new T(std::forward<A>(args)...));
}

template<class T, class Ptr>
unique_ptr<SelectProvider<T,Ptr>> makeSelectProvider(Ptr& v, const string& name, vector<pair<string,shared_ptr<T>>> choises) {
	return makeUnique<SelectProvider<T,Ptr>>(v,name,choises);
}

template<class T>
struct DefaultWidgetProvider;

template<>
struct DefaultWidgetProvider<string> {typedef TextWidgetProvider type;};

template<>
struct DefaultWidgetProvider<int> {typedef NumWidgetProvider<int> type;};
template<>
struct DefaultWidgetProvider<long long> {typedef NumWidgetProvider<long long> type;};
template<>
struct DefaultWidgetProvider<int64_t> {typedef NumWidgetProvider<int64_t> type;};
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
