#ifndef X17_STD_FUNCTION_CXX17
#define X17_STD_FUNCTION_CXX17

////////////////////////////////////////////////////////////
/// Headers
////////////////////////////////////////////////////////////
#include <sstream>
#include <exception>
#include <memory>
#include <array>
#include <type_traits>
#include <cassert>

// github.com/execorn/std_function_cxx
namespace X17 {

////////////////////////////////////////////////////////////

struct bad_function_call : std::runtime_error {
   public:
    bad_function_call(const char* file, size_t line_number)
        : std::runtime_error(report_message) {
        // This check probably should be here, but my throw_bad_call macro
        // guarantees __FILE__ will be passed as the second argument
        
        assert(file != nullptr);
       //TODO: change to sprintf 
        std::ostringstream exception_message_stream;
        exception_message_stream << file << ":" << line_number << ": "
                                 << report_message;

        m_message = exception_message_stream.str();
    }

    ~bad_function_call() throw() {}

    const char* what() const throw() { return m_message.c_str(); }

   private:
    const std::string report_message = "Bad function call";
    std::string m_message;
};

#define throw_bad_call() throw bad_function_call(__FILE__, __LINE__);

////////////////////////////////////////////////////////////

template <typename>
class function; /* undefined class */

////////////////////////////////////////////////////////////

template <typename TResult, typename... /* useless for typedefs */>
class function_typedefs_initializer {
   public:
    using return_type = TResult;
};

template <typename TResult, typename TArg>
class function_typedefs_initializer<TResult, TArg> {
   public:
    using return_type = TResult;
    using argument_type = TArg;
};

template <typename TResult, typename T1Arg, typename T2Arg>
class function_typedefs_initializer<TResult, T1Arg, T2Arg> {
   public:
    using return_type = TResult;
    using first_argument_type = T1Arg;
    using second_argument_type = T2Arg;
};

////////////////////////////////////////////////////////////

template <typename TResult, typename... TArgs>
class function_wrapper_base {
   public:
    virtual ~function_wrapper_base() noexcept {}

    virtual TResult operator()(TArgs&&... arguments) = 0;

    virtual void copy(void* ptr_to_dest) const = 0;

    virtual function_wrapper_base<TResult, TArgs...>* clone() const = 0;
};

////////////////////////////////////////////////////////////

template <typename Functor, typename TResult, typename... TArgs>
class function_wrapper final : function_wrapper_base<TResult, TArgs...> {
   public:
    function_wrapper(Functor function) noexcept : m_functor(function) {}

    TResult operator()(TArgs&&... arguments) override final {
        return m_functor(std::forward<TArgs>(arguments)...);
    }

    // TODO: check if noexcept is needed
    void copy(void* ptr_to_dest) const override final {
        new (ptr_to_dest) function_wrapper(m_functor);
    }

    function_wrapper_base<TResult, TArgs...>* clone()
        const override final {
        return new function_wrapper(m_functor);
    }

    Functor m_functor;
};

////////////////////////////////////////////////////////////

template <typename TResult, typename... TArgs>
class function<TResult(TArgs...)>
    : public function_typedefs_initializer<TResult, TArgs...> {
   public:
    function() noexcept = default;

    template <typename Functor>
    function(Functor functor) noexcept {
        if constexpr (sizeof(function_wrapper<Functor, TResult, TArgs...>) <=
                      sizeof(m_stack)) {
            m_function_wrapper_ptr =
                (decltype(m_function_wrapper_ptr))std::addressof(m_stack);
            new (m_function_wrapper_ptr)
                function_wrapper<Functor, TResult, TArgs...>(functor);
        } else {
            m_function_wrapper_ptr =
                new function_wrapper<Functor, TResult, TArgs...>(functor);
        }
    }

    function(const function& other) noexcept {
        if (other.m_function_wrapper_ptr) {
            if (other.m_function_wrapper_ptr ==
                (decltype(other.m_function_wrapper_ptr))std::addressof(
                    other.m_stack)) {
                m_function_wrapper_ptr =
                    (decltype(m_function_wrapper_ptr))std::addressof(m_stack);
                other.m_function_wrapper_ptr->copy(m_function_wrapper_ptr);
            } else {
                m_function_wrapper_ptr = other.m_function_wrapper_ptr->clone();
            }
        }
    }

    function& operator=(function const& other) {
        if (m_function_wrapper_ptr != nullptr) {
            if (m_function_wrapper_ptr ==
                (decltype(m_function_wrapper_ptr))std::addressof(m_stack)) {
                // TODO: resolve the warning -Wmaybe-uninitialized (leak
                // sanitizer)
                // TODO: add pragmas to silent warning
                m_function_wrapper_ptr->~function_wrapper_base();
            } else {
                delete m_function_wrapper_ptr;
            }

            m_function_wrapper_ptr = nullptr;
        }

        if (other.m_function_wrapper_ptr != nullptr) {
            if (other.m_function_wrapper_ptr ==
                (decltype(other.m_function_wrapper_ptr))std::addressof(
                    other.m_stack)) {
                m_function_wrapper_ptr =
                    (decltype(m_function_wrapper_ptr))std::addressof(m_stack);
                other.m_function_wrapper_ptr->copy(m_function_wrapper_ptr);
            } else {
                m_function_wrapper_ptr = other.m_function_wrapper_ptr->clone();
            }
        }

        return *this;
    }

    ~function() {
        if (m_function_wrapper_ptr ==
            (decltype(m_function_wrapper_ptr))std::addressof(m_stack)) {
            m_function_wrapper_ptr->~function_wrapper_base();
        } else {
            delete m_function_wrapper_ptr;
        }
    }

    TResult operator()(TArgs&&... arguments) const {
        if (m_function_wrapper_ptr == nullptr) {
            throw_bad_call();
        }
        return (*m_function_wrapper_ptr)(std::forward<TArgs>(arguments)...);
    }

   private:
    function_wrapper_base<TResult, TArgs...>* m_function_wrapper_ptr = nullptr;
    typename std::aligned_storage<32>::type m_stack;
};

}  // namespace X17

#endif  // !X17_STD_FUNCTION_CXX17