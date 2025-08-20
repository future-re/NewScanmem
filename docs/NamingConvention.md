# **C++ 命名规范**

## **1. 类和结构体**

- **类名**：使用 `CamelCase` 风格。
  - 示例：

    ```cpp
    class MyClassName {};
    ```

- **结构体名**：使用 `CamelCase` 风格。
  - 示例：

    ```cpp
    struct MyStructName {};
    ```

---

## **2. 枚举**

- **枚举类型名**：使用 `CamelCase` 风格。
  - 示例：

    ```cpp
    enum MyEnumType {};
    ```

- **枚举常量名**：使用 `UPPER_CASE` 风格。
  - 示例：

    ```cpp
    enum MyEnumType {
        ENUM_CONSTANT_ONE,
        ENUM_CONSTANT_TWO
    };
    ```

---

## **3. 函数**

- **函数名**：使用 `camelBack` 风格。
  - 示例：

    ```cpp
    void myFunctionName();
    ```

---

## **4. 变量**

- **普通变量名**：使用 `camelBack` 风格。
  - 示例：

    ```cpp
    int myVariableName;
    ```

- **全局变量名**：使用 `UPPER_CASE` 风格。
  - 示例：

    ```cpp
    int GLOBAL_VARIABLE_NAME;
    ```

- **常量名**：使用 `UPPER_CASE` 风格。
  - 示例：

    ```cpp
    const int MAX_BUFFER_SIZE = 1024;
    ```

---

## **5. 参数**

- **函数参数名**：使用 `camelBack` 风格。
  - 示例：

    ```cpp
    void myFunction(int parameterName);
    ```

---

## **6. 命名空间**

- **命名空间名**：使用 `lower_case` 风格。
  - 示例：

    ```cpp
    namespace my_namespace {
        void myFunction();
    }
    ```

---

## **7. 宏**

- **宏定义名**：使用 `UPPER_CASE` 风格。
  - 示例：

    ```cpp
    #define MAX_BUFFER_SIZE 1024
    ```

---

## **8. 成员变量**

- **私有成员变量**：以 `m_` 为前缀，使用 `camelBack` 风格。
  - 示例：

    ```cpp
    class MyClass {
    private:
        int m_privateMember;
    };
    ```

- **受保护成员变量**：以 `m_` 为前缀，使用 `camelBack` 风格。
  - 示例：

    ```cpp
    class MyClass {
    protected:
        int m_protectedMember;
    };
    ```

- **静态成员变量**：以 `s_` 为前缀，使用 `camelBack` 风格。
  - 示例：

    ```cpp
    class MyClass {
    private:
        static int s_staticMember;
    };
    ```

- **公共成员变量**：使用 `camelBack` 风格。
  - 示例：

    ```cpp
    class MyClass {
    public:
        int publicMemberVariable;
    };
    ```

---

## **9. 特殊规则**

- **禁用的规则**：
  - 禁用 `modernize-use-trailing-return-type`。
  - 禁用 `modernize-avoid-c-arrays`。

---
