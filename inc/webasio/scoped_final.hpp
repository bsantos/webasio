//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

namespace webasio {

template<class F>
class scoped_final {
public:
    scoped_final(F&& f)
        : f_ { std::move(f) }
    {}

    ~scoped_final()
    {
        if (!cancel_)
            f_();
    }

    void cancel()
    {
        cancel_ = true;
    }

private:
    bool cancel_ = false;
    F f_;
};

} // namespace webasio
