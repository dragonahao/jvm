
#include "java\object.h"
#include "java\jvm.h"
#include "java\clazz.h"

using namespace java;

object::object(const char* str)
    : _type(jobject_value)
{
    _ref = jni::new_string_utf(str);
    _value.l = _ref.get();
}

object::object(const clazz& cls)
    : _type(jobject_value)
{
    _ref = cls.ref();
    _value.l = _ref.get();
}

void object::make_global()
{
    if (_type == jobject_value)
    {
        _ref.make_global();
        _value.l = _ref.get();
    }
}

clazz object::get_clazz()
{
    switch (_type)
    {
    case jboolean_value: return jni::find_class("java/lang/Boolean");
    case jbyte_value: return jni::find_class("java/lang/Byte");
    case jchar_value: return jni::find_class("java/lang/Character");
    case jshort_value: return jni::find_class("java/lang/Short");
    case jint_value: return jni::find_class("java/lang/Integer");
    case jlong_value: return jni::find_class("java/lang/Long");
    case jfloat_value: return jni::find_class("java/lang/Float");
    case jdouble_value: return jni::find_class("java/lang/Double");
    case jobject_value: return _value.l == nullptr ? clazz() : clazz(jni::get_object_class(native()));
    default:
        throw std::exception("Unsupported Java type");
    }
}

jsize object::array_size()
{
    if (_type != jobject_value) throw std::exception("Not an object type");
    return jni::get_array_length((jarray)native());
}

array_element object::operator[](size_t index)
{
    auto class_name = get_clazz().name();
    if (class_name.size() < 2 || class_name[0] != '[') throw std::exception("Not an array type");

    switch (class_name[1])
    {
    case 'Z': return array_element(*this, get_element<jboolean>(index), index); break;
    case 'B': return array_element(*this, get_element<jbyte>(index), index); break;
    case 'C': return array_element(*this, get_element<jchar>(index), index); break;
    case 'L': return array_element(*this, jni::get_object_array_element((jobjectArray)_ref.get(), index), index); break;
    case 'D': return array_element(*this, get_element<jdouble>(index), index); break;
    case 'F': return array_element(*this, get_element<jfloat>(index), index); break;
    case 'I': return array_element(*this, get_element<jint>(index), index); break;
    case 'J': return array_element(*this, get_element<jlong>(index), index); break;
    case 'S': return array_element(*this, get_element<jshort>(index), index); break;
    default:
        throw std::exception("Unsupported Java type");
    }
}

std::string object::to_string()
{
    auto toString = jni::get_method_id(get_clazz().native(), "toString", "()Ljava/lang/String;");
    local_ref<jstring> jstr(jni::call_method<jobject>(_value.l, toString));
    return jstring_str(jstr.get());
}

object object::field(const char* name)
{
    return get_clazz().call_static("getField", name).call("get", *this);
}

object call_method(jobject obj, std::string& return_type, jmethodID id, ...)
{
    va_list args;
    va_start(args, id);

    object ret;

    try
    {
        auto rtype = return_type;
        if (return_type == "void")
        {
            jni::call_methodv<void>(obj, id, args);
            ret = object();
        }
        else if (rtype == "boolean")
            ret = object(jni::call_methodv<jboolean>(obj, id, args));
        else if (rtype == "byte")
            ret = object(jni::call_methodv<jbyte>(obj, id, args));
        else if (rtype == "char")
            ret = object(jni::call_methodv<jchar>(obj, id, args));
        else if (rtype == "double")
            ret = object(jni::call_methodv<jdouble>(obj, id, args));
        else if (rtype == "float")
            ret = object(jni::call_methodv<jfloat>(obj, id, args));
        else if (rtype == "int")
            ret = object(jni::call_methodv<jint>(obj, id, args));
        else if (rtype == "long")
            ret = object(jni::call_methodv<jlong>(obj, id, args));
        else if (rtype == "short")
            ret = object(jni::call_methodv<jshort>(obj, id, args));
        else
            ret = object(jni::call_methodv<jobject>(obj, id, args));
    }
    catch (...)
    {
        va_end(args);
        throw;
    }

    va_end(args);

    return ret;
}

// These functions call Java methods on the object given the method 
// name and a number of arguments.  An exception is thrown if an 
// appropriate method is not found.
object object::call(const char* method_name)
{
    std::vector<clazz> classes;
    auto m = get_clazz().lookup_method(method_name, classes);
    return call_method(_value.l, m.return_type(), m.id());
}
object object::call(const char* method_name, object a1)
{
    std::vector<clazz> classes;
    classes.push_back(a1.get_clazz());
    auto m = get_clazz().lookup_method(method_name, classes);
    return call_method(_value.l, m.return_type(), m.id(), a1.native());
}
object object::call(const char* method_name, object a1, object a2)
{
    std::vector<clazz> classes;
    classes.push_back(a1.get_clazz());
    classes.push_back(a2.get_clazz());
    auto m = get_clazz().lookup_method(method_name, classes);
    return call_method(_value.l, m.return_type(), m.id(), a1.native(), a2.native());
}
object object::call(const char* method_name, object a1, object a2, object a3)
{
    std::vector<clazz> classes;
    classes.push_back(a1.get_clazz());
    classes.push_back(a2.get_clazz());
    classes.push_back(a3.get_clazz());
    auto m = get_clazz().lookup_method(method_name, classes);
    return call_method(_value.l, m.return_type(), m.id(), a1.native(), a2.native(), a3.native());
}

array_element& array_element::operator= (const object& rhs)
{
    switch (_type)
    {
    case jboolean_value: set<jboolean>(rhs.as_bool()); break;
    case jbyte_value: set(rhs.as_byte()); break;
    case jchar_value: set(rhs.as_char()); break;
    case jobject_value: set(rhs.native()); break;
    case jfloat_value: set(rhs.as_float()); break;
    case jdouble_value: set(rhs.as_double()); break;
    case jint_value: set(rhs.as_int()); break;
    case jlong_value: set(rhs.as_long()); break;
    case jshort_value: set(rhs.as_short()); break;
    default:
        throw std::exception("Unsupported Java type");
    }

    return *this;
}

namespace java
{

    object create(const char* class_name)
    {
        clazz cls(class_name);
        std::vector<clazz> classes;
        auto ctor = cls.lookup_constructor(classes);
        return jni::new_object(cls.native(), ctor.id());
    }

    object create(const char* class_name, object a1)
    {
        clazz cls(class_name);
        std::vector<clazz> classes;
        classes.push_back(a1.get_clazz());
        auto ctor = cls.lookup_constructor(classes);
        return jni::new_object(cls.native(), ctor.id(), a1.native());
    }

    object create(const char* class_name, object a1, object a2)
    {
        clazz cls(class_name);
        std::vector<clazz> classes;
        classes.push_back(a1.get_clazz());
        classes.push_back(a2.get_clazz());
        auto ctor = cls.lookup_constructor(classes);
        return jni::new_object(cls.native(), ctor.id(), a1.native(), a2.native());
    }

    object create(const char* class_name, object a1, object a2, object a3)
    {
        clazz cls(class_name);
        std::vector<clazz> classes;
        classes.push_back(a1.get_clazz());
        classes.push_back(a2.get_clazz());
        classes.push_back(a3.get_clazz());
        auto ctor = cls.lookup_constructor(classes);
        return jni::new_object(cls.native(), ctor.id(), a1.native(), a2.native(), a3.native());
    }

    // Creates a new object (non-primitive) array
    object create_array(const char* class_name, size_t size, object initial)
    {
        clazz cls(class_name);
        return object(jni::new_object_array(cls.native(), size, initial.native()));
    }

}
