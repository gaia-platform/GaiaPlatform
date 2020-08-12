#include "gtest/gtest.h"
#include <memory>
#include "gaia_expression.hpp"

class fake_obj;
class obj_holder_t;
class field_holder_t;
class string_holder_t;
class float_holder_t;

using namespace gaia::expr;


class fake_obj {
    public:
    int integer() const{
        return 2;
    }
    std::string hello() const {
        return std::string("hello");
    }

    float f() const {
        return 1.234;
    }

    static const obj_holder_t fake_obj_;
    static const field_holder_t integer_;
    static const string_holder_t str_;
    static const float_holder_t float_;
};

class obj_holder_t : public variable_expression_t<fake_obj&, fake_obj> {
    public:
    fake_obj& eval(fake_obj& obj) const override {
        return obj;
    }
};

class field_holder_t: public variable_expression_t<int, fake_obj> {
    public:
    int eval(fake_obj& obj) const override {
        return obj.integer();
    }
};

class string_holder_t: public variable_expression_t<std::string, fake_obj> {
    public:
    std::string eval(fake_obj& obj) const override {
        return obj.hello();
    }
};

class float_holder_t: public variable_expression_t<float, fake_obj> {
    public:
    float eval(fake_obj& obj) const override {
        return obj.f();
    }
};

const obj_holder_t fake_obj::fake_obj_;
const field_holder_t fake_obj::integer_;
const string_holder_t fake_obj::str_;
const float_holder_t fake_obj::float_;

class coords_obj_t;
class coords_x_t;
class coords_y_t;

class drone_obj_t;
class drone_pos_t;

class drone_pos_access_x_t;
class drone_pos_access_y_t;

class coords {
    public:
    int x;
    int y;

    static const coords_obj_t self_;
    static const coords_x_t x_;
    static const coords_y_t y_;

    bool operator==(const coords& other) const {
        return other.x == x && other.y == y;
    }
};

class drone {
    public:
    coords pos;

    static const drone_obj_t self_;
    static const drone_pos_t pos_;
};

//shadow drone
namespace dsl {
struct drone {
    static const drone_obj_t self;
    static const drone_pos_t pos;
};
}

class coords_obj_t: public variable_expression_t<coords&, coords> {
    public:
    coords& eval(coords& obj) const override {
        return obj;
    }
};

class coords_x_t: public variable_expression_t<int, coords> {
    public:
    int eval(coords& obj) const override {
        return obj.x;
    }
};

class coords_y_t: public variable_expression_t<int, coords> {
    public:
    int eval(coords& obj) const override {
        return obj.y;
    }
};

const coords_obj_t coords::self_;
const coords_x_t coords::x_;
const coords_y_t coords::y_;

class drone_obj_t : public variable_expression_t<drone&, drone> {
    public:
    drone& eval(drone& obj) const override {
        return obj;
    }
};

class drone_pos_t : public variable_expression_t<coords&, drone> {
    public:
    static const interpreter<coords> pos_access;
    const static drone_pos_access_x_t x_;
    const static drone_pos_access_y_t y_;

    coords& eval(drone& obj) const override {
        return obj.pos;
    }
};


class drone_pos_access_x_t : public field_access_expression_t<int, coords, drone, coords_x_t> {
    public:
    coords& access(drone& obj) const override {
        return obj.pos;
    }
};

class drone_pos_access_y_t : public field_access_expression_t<int, coords, drone, coords_y_t> {
    public:
    coords& access(drone& obj) const override {
        return obj.pos;
    }
};

const drone_obj_t drone::self_;
const drone_pos_t drone::pos_;

const drone_obj_t dsl::drone::self;
const drone_pos_t dsl::drone::pos;

const drone_pos_access_x_t drone_pos_t::x_;
const drone_pos_access_y_t drone_pos_t::y_;

// a function:
int half(int x) {return x/2;}

struct tmp{
    std::function<int(int)>f;
    tmp(std::function<int(int)> f) :
        f(f){}
};

TEST(expr, basic_field_eval) {
    fake_obj obj;
    interpreter<fake_obj> interp;
    interp.bind(obj);

    std::string my_string("hello");
    gaia::expr::native_expression_t<int> two(2);

    evaluable_function<int(int)> eval_half(half);

    static_assert(std::is_same<expr_type<decltype(fake_obj::integer_)>, int>::value, "underlying broken");

    ASSERT_EQ(interp.eval(fake_obj::integer_), 2);
    ASSERT_EQ(interp.eval(two), 2);
    ASSERT_TRUE(interp.eval(fake_obj::integer_ == 2));
    ASSERT_TRUE(interp.eval(fake_obj::integer_ == fake_obj::integer_));
    ASSERT_TRUE(interp.eval(2 == fake_obj::integer_));
    ASSERT_FALSE(interp.eval(22 == fake_obj::integer_));
    ASSERT_TRUE(interp.eval(fake_obj::str_ == "hello"));
    ASSERT_FALSE(interp.eval("bye" == fake_obj::str_));
    ASSERT_TRUE(interp.eval(fake_obj::str_ == my_string));
    ASSERT_EQ(interp.eval(fake_obj::integer_ + 1), 3);
    ASSERT_EQ(interp.eval(fake_obj::integer_ - 1), 1);
    ASSERT_EQ(interp.eval(eval_half(2)), 1);
    ASSERT_EQ(interp.eval(eval_half(fake_obj::integer_)), 1);
}

TEST(expr, str_lexicographic_eval) {
    fake_obj obj;
    interpreter<fake_obj> interp;
    interp.bind(obj);

    std::string my_string("hello");
    std::string my_string2("nihao");
    std::string my_string3("aloha");

    ASSERT_TRUE(interp.eval(fake_obj::str_ > "aloha"));
    ASSERT_TRUE(interp.eval(fake_obj::str_ <= "hello"));
    ASSERT_FALSE(interp.eval("nihao" <= fake_obj::str_));
    ASSERT_FALSE(interp.eval(my_string2 <= fake_obj::str_));
    ASSERT_TRUE(interp.eval("aloha" <= fake_obj::str_));
}

TEST(expr, mixed_types_eval) {
    fake_obj obj;
    interpreter<fake_obj> interp;
    interp.bind(obj);

    ASSERT_TRUE(interp.eval(fake_obj::integer_ < 3.141));
    ASSERT_TRUE(interp.eval(fake_obj::integer_ > -1));

    ASSERT_TRUE(interp.eval(4 > fake_obj::float_));
    ASSERT_TRUE(interp.eval(3.141 > fake_obj::float_));

    ASSERT_FALSE(interp.eval(fake_obj::float_ < -fake_obj::float_));
    ASSERT_TRUE(interp.eval(fake_obj::integer_ != fake_obj::float_));
    ASSERT_FALSE(interp.eval(!(fake_obj::integer_ != fake_obj::float_)));

    ASSERT_EQ(interp.eval(gaia::expr::cast<int>(fake_obj::float_) << 1), 2);
}

TEST(expr, drone_test) {
    drone d;
    interpreter<drone> interp;
    interp.bind(d);

    d.pos.x = 100;
    d.pos.y = 20;

    coords crash_site = {33, 44};

    ASSERT_TRUE(interp.eval(drone::pos_.x_ == 100));
    ASSERT_FALSE(interp.eval(drone::pos_.y_ + 500 != 520));
    ASSERT_FALSE(d.pos == crash_site);
    ASSERT_FALSE(interp.eval(dsl::drone::pos == crash_site));
}
