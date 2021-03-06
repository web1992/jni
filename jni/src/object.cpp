
#include <jni/java/lang/object.hpp>

#include <iostream>
#include <utility>

#include <jni/environment.hpp>
#include <jni/java/lang/class.hpp>
#include <jni/raw/method.hpp>
#include <jni/string.hpp>

namespace jni {
namespace java {
namespace lang {

namespace {

struct method_cache {
  raw::method<std::int32_t()> hashCode;
  raw::method<local_ref<raw::string_ref>()> toString;
  raw::method<bool(local_ref<raw::object_ref>)> equals;
  raw::method<void()> notify;
  raw::method<void()> notifyAll;
  raw::method<void()> wait1;
  raw::method<void(std::int64_t timeout)> wait2;
  raw::method<void(std::int64_t timeout, std::int32_t nanoseconds)> wait3;

  method_cache(Class cls)
      : hashCode{cls, "hashCode"}, toString{cls, "toString"},
        equals{cls, "equals"}, notify{cls, "notify"},
        notifyAll{cls, "notifyAll"}, wait1{cls, "wait"}, wait2{cls, "wait"},
        wait3{cls, "wait"} {}

  static method_cache &get() {
    static method_cache cache{Class::forName("java/lang/Object")};
    return cache;
  }
};

} // namespace anonymous

//
// Object
//

Object::Object(local_ref<raw::object_ref> &&ref) : _ref(std::move(ref)) {}
Object::Object(global_ref<raw::object_ref> &&ref) : _ref(std::move(ref)) {}

Object::Object(const Object &other) = default;
Object::Object(Object &&other) = default;

Object::~Object() = default;

Object &Object::operator=(const Object &obj) = default;
Object &Object::operator=(Object &&obj) = default;

int Object::hashCode() const { return method_cache::get().hashCode(*this); }

std::string Object::toString() const {
  return to_string(method_cache::get().toString(*this));
}

bool Object::equals(const Object &other) const {
  return method_cache::get().equals(*this, other._ref.raw());
}

Class Object::getClass() const {
  return Class{environment::current().get_object_class(_ref.raw())};
}

void Object::notify() { method_cache::get().notify(*this); }

void Object::notifyAll() { method_cache::get().notify(*this); }

void Object::wait() const { method_cache::get().wait1(*this); }

void Object::wait(const boost::chrono::milliseconds &timeout) const {
  method_cache::get().wait2(*this, timeout.count());
}

void Object::wait(const boost::chrono::nanoseconds &timeout) const {
  auto milliseconds =
      boost::chrono::duration_cast<boost::chrono::milliseconds>(timeout);
  auto remainder = timeout % 1000000;

  method_cache::get().wait3(*this, milliseconds.count(),
                            static_cast<std::int32_t>(remainder.count()));
}

raw::object_ref extract_reference(const Object &object) {
  return object._ref.raw();
}

bool operator==(const Object &left, const Object &right) {
  return left.equals(right);
}

bool operator!=(const Object &left, const Object &right) {
  return !(left == right);
}

bool operator==(const Object &left, std::nullptr_t) {
  return extract_reference(left) == nullptr;
}

bool operator==(std::nullptr_t, const Object &right) {
  return nullptr == extract_reference(right);
}

bool operator!=(const Object &left, std::nullptr_t) {
  return extract_reference(left) != nullptr;
}

bool operator!=(std::nullptr_t, const Object &right) {
  return nullptr != extract_reference(right);
}

std::ostream &operator<<(std::ostream &os, const Object &object) {
  return os << object.toString();
}

//
// Monitor
//

Monitor::Monitor(Object &obj) : _obj(&obj), _entered(false) { enter(); }

Monitor::Monitor(Object &obj, defer_t) : _obj(&obj), _entered(false) {}

Monitor::Monitor(Monitor &&other) : _obj(other._obj), _entered(other._entered) {
  other._obj = nullptr;
  other._entered = false;
}

Monitor::~Monitor() { leave(); }

void Monitor::enter() {
  if (_entered || !_obj)
    return;

  environment::current().monitor_enter(_obj->_ref.raw());

  _entered = true;
}

void Monitor::leave() {
  if (!_entered || !_obj)
    return;

  environment::current().monitor_leave(_obj->_ref.raw());

  _entered = false;
}

} // namespace lang
} // namespace java
} // namespace jni
