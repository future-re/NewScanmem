# **C++ Naming Conventions**

## **1. Classes and Structures**

- **Class Names**: Use `CamelCase` style.
  - Example:

    ```cpp
    class MyClassName {};
    ```

- **Structure Names**: Use `CamelCase` style.
  - Example:

    ```cpp
    struct MyStructName {};
    ```

---

## **2. Enums**

- **Enum Type Names**: Use `CamelCase` style.
  - Example:

    ```cpp
    enum MyEnumType {};
    ```

- **Enum Constant Names**: Use `UPPER_CASE` style.
  - Example:

    ```cpp
    enum MyEnumType {
        ENUM_CONSTANT_ONE,
        ENUM_CONSTANT_TWO
    };
    ```

---

## **3. Functions**

- **Function Names**: Use `camelBack` style.
  - Example:

    ```cpp
    void myFunctionName();
    ```

---

## **4. Variables**

- **Regular Variable Names**: Use `camelBack` style.
  - Example:

    ```cpp
    int myVariableName;
    ```

- **Global Variable Names**: Use `UPPER_CASE` style.
  - Example:

    ```cpp
    int GLOBAL_VARIABLE_NAME;
    ```

- **Constant Names**: Use `UPPER_CASE` style.
  - Example:

    ```cpp
    const int MAX_BUFFER_SIZE = 1024;
    ```

---

## **5. Parameters**

- **Function Parameter Names**: Use `camelBack` style.
  - Example:

    ```cpp
    void myFunction(int parameterName);
    ```

---

## **6. Namespaces**

- **Namespace Names**: Use `lower_case` style.
  - Example:

    ```cpp
    namespace my_namespace {
        void myFunction();
    }
    ```

---

## **7. Macros**

- **Macro Names**: Use `UPPER_CASE` style.
  - Example:

    ```cpp
    #define MAX_BUFFER_SIZE 1024
    ```

---

## **8. Member Variables**

- **Private Member Variables**: Prefix with `m_` and use `camelBack` style.
  - Example:

    ```cpp
    class MyClass {
    private:
        int m_privateMember;
    };
    ```

- **Protected Member Variables**: Prefix with `m_` and use `camelBack` style.
  - Example:

    ```cpp
    class MyClass {
    protected:
        int m_protectedMember;
    };
    ```

- **Static Member Variables**: Prefix with `s_` and use `camelBack` style.
  - Example:

    ```cpp
    class MyClass {
    private:
        static int s_staticMember;
    };
    ```

- **Public Member Variables**: Use `camelBack` style.
  - Example:

    ```cpp
    class MyClass {
    public:
        int publicMemberVariable;
    };
    ```

---

## **9. Special Rules**

- **Disabled Rules**:
  - Disable `modernize-use-trailing-return-type`.
  - Disable `modernize-avoid-c-arrays`.

---