#pragma once

#include "jvm.h"

namespace java
{
    class method;
    class method_list;
    class object;

    // Wrapper for a Java class object. "clazz" naming is to avoid conflict 
    // with C++ class keyword.
    class clazz
    {
        local_ref<jclass> _ref;

    public:
        // Returns a clazz object that refers to a the Java class of a null 
        // pointer (there is none, so internally is just a clazz object with 
        // jclass value that is 0.
        clazz();

        // This constructor looks up a Java class given it's name.  The 
        // format of the name is 
        // "namespace1/namespace2/..../namespaceN/class_name", e.g., 
        // "java/lang/String".  Generic types don't have any special notation,
        // e.g., java.util.ArrayList<E> is just "java/util/ArrayList".  This 
        // is because the JVM doesn't have any notion of generics- it is only 
        // known at the compiler level.
        clazz(const char* name);

        // Returns a clazz object that corresponds to the specified object.  
        // The object should be an instance of a java.lang.Class object.
        clazz(object);

        clazz(jclass cls) : _ref(cls) {}

        local_ref<jclass> ref() const { return _ref; }
        jclass native() const { return _ref.get(); }

        std::string name();

        // Returns a class field value with the given name.
        object static_field(const char* name);

        // Finds a method that is callable, given the set of classes as 
        // method arguments.  A method is considered appropriate if it has 
        // the correct name, correct number of arguments, and each class is 
        // assignable to the corresponding method argument.
        method lookup_method(const char* name, const std::vector<clazz>& classes);

        // Does the same as lookup_method, but for constructors.
        method lookup_constructor(const std::vector<clazz>& classes);

        // Returns a list of methods for this class using reflection.
        method_list get_methods();

        // Returns a list of constructors for this class using reflection.
        method_list get_constructors();

        // These methods call static Java methods on the class given the 
        // method name and a number of arguments.  An exception is thrown
        // if an appropriate method is not found.  Also, the first method 
        // which accepts the given arguments is called, even if there is a 
        // "better" match.  For now, if it is necessary to disambiguate, the
        // low-level jni::call_xxx_method() functions must be used.
        object call_static (const char* method_name);
        object call_static (const char* method_name, object a1);
        object call_static (const char* method_name, object a1, object a2);
        object call_static (const char* method_name, object a1, object a2, object a3);
    };

    // This function can be used to load classes from raw compiled class data 
    // (e.g., content of a .class file generated by javac).  The system class 
    // loader (as returned by java.lang.ClassLoader.getSystemClassLoader()) 
    // is used to load the data into the JVM.
    clazz load_class(const char* class_name, jbyte* class_data, jsize size);
}
