/**
 * hello_world.cc
 *
 */

#include "ChakraCore.h"
#include <iostream>

/**
 * JsGetAndClearException 负责获取异常结果并清除 Runtime 的异常状态
 * 获取的异常结果是一个对象，对象有一个 message 属性保存错误信息，
 * 所以需要创建一个 id 来读取这个属性值
 */
#define ErrorCheck(cmd)                                                  \
    do {                                                                 \
        JsErrorCode errCode = cmd;                                       \
        if (errCode != JsNoError && errCode == JsErrorScriptException) { \
            JsValueRef exception;                                        \
            JsGetAndClearException(&exception);                          \
                                                                         \
            JsPropertyIdRef id;                                          \
            JsCreatePropertyId("message", sizeof("message") - 1, &id);   \
                                                                         \
            JsValueRef value;                                            \
            JsGetProperty(exception, id, &value);                        \
                                                                         \
            return JsValueRefToStr(value);                               \
        }                                                                \
    } while (0)

std::unique_ptr<char[]> JsValueRefToStr(JsValueRef JsValue);
JsValueRef Echo(JsValueRef callee, bool isConstructCall, JsValueRef* arguments, unsigned short argumentCount, void* callbackState);

class ChakraHandle {
private:
    JsRuntimeHandle runtime;
    JsContextRef context;
    int callTimes = 0;

public:
    ChakraHandle()
    {
        /**
         * 创建一个 Runtime，Runtime 是一个完整的 JavaScript 执行环境，包含自己的 JIT, GC，
         * 一个特定的线程中只能有一个活跃的 Runtime
         */
        JsCreateRuntime(JsRuntimeAttributeNone, nullptr, &runtime);

        /**
         * 创建一个 Context，一个 Runtime 可包含多个 Context，多个 Context 共享 JIT，GC
         */
        JsCreateContext(runtime, &context);
        JsSetCurrentContext(context);

        setEcho();
    }

    ~ChakraHandle()
    {
        /**
         * 引擎运行结束后回收资源
         */
        JsSetCurrentContext(JS_INVALID_REFERENCE);
        JsDisposeRuntime(runtime);
    }

    /**
     * 通过此函数来执行一段 JavaScript 字符串
     */
    std::unique_ptr<char[]> Eval_JS(const char* script, const char* sourceUrl = "")
    {
        /**
         * JsValueRef 是一个 JavaScript 值的引用，外部资源与引擎内的 JavaScript 资源交互
         * 都需要通过它
         */

        /**
         * 运行一段 JS 代码时需要告知引擎这段代码的来源，通常为空字符串即可。
         * JsSourceUrl 保存 sourceUrl 转化为 JavaScript 的值引用
         */
        JsValueRef JsSourceUrl;
        JsCreateString(sourceUrl, strlen(sourceUrl), &JsSourceUrl);

        /**
         * ChakraCore 运行一段 JS 代码时对脚本字符串数据类型要求为
         * JavascriptString 或 JavascriptExternalArrayBuffer，
         * 使用 JavascriptExternalArrayBuffer 有更好的性能及更小的内存消耗
         *
         * JsScript 保存将原始的 const char* 转换为 ArrayBuffer 后的值引用
         */
        JsValueRef JsScript;
        JsCreateExternalArrayBuffer((void*)script, strlen(script), nullptr, nullptr, &JsScript);

        /**
         * 终于要运行了！
         * JsParseScriptAttributeNone 表示我们不对 Parse 过程有任何设置
         * callTimes 是一个标识运行过程的cookie，仅调试用，这里我们给一个自增的int即可
         */
        JsValueRef result;
        ErrorCheck(JsRun(JsScript, callTimes++, JsSourceUrl, JsParseScriptAttributeNone, &result));

        /**
         * 返回脚本的运行结果，这时 result 还是 JavaScript 值引用，需要将其转换为字符串
         */
        return JsValueRefToStr(result);
    }

    void setEcho()
    {
        /**
         * 创建要挂载到全局对象内的函数
         */
        JsValueRef function;
        JsCreateFunction(Echo, nullptr, &function);

        /**
         * 获取全局对象引用
         */
        JsValueRef globalObject;
        JsGetGlobalObject(&globalObject);

        /**
         * 创建函数名，因为是在全局对象下，所以实际上是一个属性名
         */
        JsPropertyIdRef func_name;
        JsCreatePropertyId("echo", sizeof("echo") - 1, &func_name);

        /**
         * 挂载！
         */
        JsSetProperty(globalObject, func_name, function, false);
    }
};

/**
 * 这是一个 JsNativeFunction，也就是 JS 调用时实际执行的部分
 */
JsValueRef Echo(JsValueRef callee, bool isConstructCall, JsValueRef* arguments, unsigned short argumentCount, void* callbackState)
{
    /**
     * arguments 是一个 JsValueRef 型数组，表示用户传进来的参数，不过从第一个起才是用户传进来的
     * 当参数满足需求时，我们打印第一个
     */
    if (argumentCount >= 2) {
        std::cout << JsValueRefToStr(arguments[1]).get() << std::endl;
    }

    /**
     * JS_INVALID_REFERENCE 表示一个非法的引用，对于 JS 来说函数返回了个 undefined
     */
    return JS_INVALID_REFERENCE;
}

/**
 * JavaScript 值引用到 C++ 字符串的过程
 */
std::unique_ptr<char[]> JsValueRefToStr(JsValueRef JsValue)
{
    /**
     * 值引用在 ChakraCore 中并不一定就是字符串，所以首先要将其转换为字符串
     * 如果已经是字符串，那这一步什么都不会改变
     */
    JsValueRef JsValueString;
    JsConvertValueToString(JsValue, &JsValueString);

    /**
     * 由于我们并不知道上一步转换出的字符串长度是多少，所以需要两次调用
     * JsCopyString，第一次获取长度，第二次才是真实的复制内容到 C++ 里
     */
    size_t strLength;
    JsCopyString(JsValueString, nullptr, 0, &strLength);

    auto result = std::make_unique<char[]>(strLength + 1);

    JsCopyString(JsValueString, result.get(), strLength + 1, nullptr);

    result.get()[strLength] = 0;
    return result;
}

int main()
{
    auto chakra = ChakraHandle();
    auto result = chakra.Eval_JS("echo('hello, world!!')").get();
    std::cout << result << std::endl; // hello, world!
}
