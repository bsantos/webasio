#pragma once

/**
 * @brief Turns an asynchronous call site into a resumed state in a manually
 * written coroutine-like state machine.
 *
 * @details
 * This is intended for composed ASIO operations written with a single
 * `switch` over the state variable and a `case` label for each resume point.
 *
 * This macro performs the following steps:
 *   1. Updates the resume variable to `resume_point`.
 *   2. Returns the value of the async initiation expression from the current
 *      handler, which both initiates the operation and suspends by returning.
 *   3. Resumes execution at `case resume_point:` on the next invocation.
 *
 * The initiation expression is tail-returned, so the enclosing handler's
 * return type must match the expression's type. Composed ASIO operations
 * initiate with a real handler (`self`), so the initiation returns `void` and
 * this pairs with a `void`-returning handler. When the enclosing handler
 * returns `void` but the initiation returns a non-void value that must be
 * discarded, use @ref WEBASIO_YIELD_VOID instead.
 *
 * The trailing arguments are variadic so an initiation expression containing
 * top-level commas (e.g. multiple template arguments) does not need extra
 * parentheses. At least one argument must be supplied.
 *
 * @param resume_var The state variable that tracks where the operation
 * should continue when the asynchronous call completes.
 * @param resume_point The enum value representing the next resume label.
 * @param ... The asynchronous initiation expression that accepts `self` and
 * returns immediately after initiation.
 */
#define WEBASIO_YIELD(resume_var, resume_point, ...) \
    do {                                             \
        resume_var = resume_point;                   \
        return (__VA_ARGS__);                        \
    case resume_point:                               \
        {}                                           \
    } while (false)

/**
 * @brief Variant of @ref WEBASIO_YIELD for a `void`-returning handler whose
 * initiation expression yields a value that must be discarded.
 *
 * @details
 * Evaluates the initiation expression for its side effects (the operation
 * initiation) and then returns `void` via the comma operator, regardless of
 * the type it produces. Use this when the enclosing handler returns `void`
 * but the initiation does not; for initiations that already return `void`,
 * plain @ref WEBASIO_YIELD suffices.
 *
 * As with @ref WEBASIO_YIELD, the trailing arguments are variadic so an
 * initiation expression containing top-level commas does not need extra
 * parentheses. At least one argument must be supplied.
 *
 * @param resume_var The state variable that tracks where the operation
 * should continue when the asynchronous call completes.
 * @param resume_point The enum value representing the next resume label.
 * @param ... The asynchronous initiation expression that accepts `self` and
 * returns immediately after initiation.
 */
#define WEBASIO_YIELD_VOID(resume_var, resume_point, ...) \
    WEBASIO_YIELD(resume_var, resume_point, __VA_ARGS__, static_cast<void>(0))
