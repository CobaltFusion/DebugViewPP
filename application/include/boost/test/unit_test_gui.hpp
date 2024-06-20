//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://boosttestui.wordpress.com/ for the boosttestui home page.

#ifndef BOOST_TEST_UNIT_TEST_GUI_HPP
#define BOOST_TEST_UNIT_TEST_GUI_HPP

#define init_unit_test_suite init_unit_test_suite2

#include <iostream>
#include <iomanip>
#include <boost/test/unit_test.hpp>
#include <boost/test/execution_monitor.hpp>

// Starting with 1.59, boost.test directly supports list_content and wait_for_debugger:
#if BOOST_VERSION >= 105900
#define BOOST_TEST_API_3
#endif

#ifndef BOOST_TEST_API_3
#include <boost/test/utils/runtime/cla/named_parameter.hpp>
#include <boost/test/utils/runtime/cla/parser.hpp>
#endif

#undef init_unit_test_suite

namespace boost {
namespace unit_test {
namespace gui {

class gui_observer : public test_observer
{
public:
    virtual void test_start(counter_t number_of_test_cases, test_unit_id /* root_test_unit_id */)
    {
        std::cout << "#start " << number_of_test_cases << std::endl;
    }

    virtual void test_finish()
    {
        std::cout << "#finish" << std::endl;
    }

    virtual void test_aborted()
    {
        std::cout << "#aborted" << std::endl;
    }

    virtual void test_unit_start(test_unit const& tu)
    {
        std::cout << "#unit_start " << tu.p_id << std::endl;
    }

    virtual void test_unit_finish(test_unit const& tu, unsigned long elapsed)
    {
        std::cout << "#unit_finish " << tu.p_id << " " << elapsed << std::endl;
    }

    virtual void test_unit_skipped(test_unit const& tu)
    {
        std::cout << "#unit_skipped " << tu.p_id << std::endl;
    }

    virtual void test_unit_aborted(test_unit const& tu)
    {
        std::cout << "#unit_aborted " << tu.p_id << std::endl;
    }

    virtual void assertion_result(unit_test::assertion_result ar)
    {
        std::cout << "#assertion " << (ar == AR_PASSED) << std::endl;
    }

    virtual void exception_caught(boost::execution_exception const& e)
    {
        std::cout << "#exception " << e.what() << std::endl;
    }
};

} // namespace gui
} // namespace unit_test
} // namespace boost

boost::unit_test::test_suite* init_unit_test_suite2(int argc, char* argv[]);

#ifdef BOOST_TEST_API_3

namespace boost {
namespace unit_test {

class Enable : public decorator::enable_if_impl
{
public:
    explicit Enable(bool enable) :
        m_enable(enable)
    {
    }

private:
    // decorator::base interface
    virtual void apply(boost::unit_test::test_unit& tu)
    {
        apply_impl(tu, m_enable);
    }

    virtual boost::unit_test::decorator::base_ptr clone() const
    {
        return boost::unit_test::decorator::base_ptr(new Enable(m_enable));
    }

    bool m_enable;
};

} // namespace unit_test
} // namespace boost

#define BOOST_AUTO_TEST_CASE_ENABLE(name, enable) \
    BOOST_AUTO_TEST_CASE(name, *::boost::unit_test::Enable(enable))

#ifndef BOOST_TEST_NO_GUI_INIT

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] == std::string("--gui_run"))
        {
            static boost::unit_test::gui::gui_observer observer;
            boost::unit_test::framework::register_observer(observer);

            while (i < argc)
            {
                argv[i] = argv[i + 1];
                ++i;
            }
            --argc;
            break;
        }
    }

    return init_unit_test_suite2(argc, argv);
}

extern "C" __declspec(dllexport) inline void unit_test_type_boost2()
{
}

#endif // !BOOST_TEST_NO_GUI_INIT

#else // !BOOST_TEST_API_3

namespace boost {
namespace unit_test {
namespace gui {

class test_suite_access : public test_suite
{
public:
    const std::vector<test_unit_id>& members() const
    {
        return m_members;
    }

private:
    test_suite_access();
};

class test_tree_reporter : public test_tree_visitor
{
public:
    test_tree_reporter() :
        m_indent(0)
    {
    }

private:
    virtual void visit(test_case const& tc)
    {
        std::cout << std::setw(m_indent) << "" << (tc.p_enabled ? 'C' : 'c') << tc.p_id << ":" << tc.p_name << "\n";
    }

    virtual bool test_suite_start(test_suite const& ts)
    {
        if (m_indent >= 0)
            std::cout << std::setw(m_indent) << "" << (ts.p_enabled ? 'S' : 's') << ts.p_id << ":" << ts.p_name << "\n";
        m_indent += 4;
        return true;
    }

    virtual void test_suite_finish(test_suite const&)
    {
        m_indent -= 4;
    }

    int m_indent;
};

class test_tree_enabler : public test_tree_visitor
{
public:
    explicit test_tree_enabler(const std::string& enableArg) :
        m_arg(enableArg), m_it(m_arg.begin())
    {
    }

private:
    void enable(test_unit_id id)
    {
        bool enable = m_it != m_arg.end() && (*m_it++ == '1');
        framework::get(id, tut_any).p_enabled.set(enable);
    }

    virtual void visit(test_case const& tc)
    {
        enable(tc.p_id);
    }

    virtual bool test_suite_start(test_suite const& ts)
    {
        enable(ts.p_id);
        return true;
    }

private:
    std::string m_arg;
    std::string::const_iterator m_it;
};

static const char* gui_list = "gui_list";
static const char* gui_run = "gui_run";
static const char* gui_wait = "gui_wait";

class runner
{
public:
    runner(int& argc, char** argv)
    {
        namespace cla = boost::runtime::cla;

        m_args - cla::ignore_mismatch
            << cla::named_parameter<bool>(gui_list) - (cla::prefix = "--", cla::optional)
            << cla::named_parameter<std::string>(gui_run) - (cla::prefix = "--", cla::separator = "=", cla::optional)
            << cla::named_parameter<bool>(gui_wait) - (cla::prefix = "--", cla::optional);
        m_args.parse(argc, argv);
    }

    void operator()()
    {
        if (m_args[gui_list])
            list();
        if (m_args[gui_run])
            run(m_args.get<std::string>(gui_run));
        if (m_args[gui_wait])
            wait();
    }

    static void list()
    {
        test_tree_reporter reporter;
        traverse_test_tree(framework::master_test_suite(), reporter);
        //	throw std::runtime_error("No tests"); // "Test setup error: std::runtime_error: No tests"
        throw framework::nothing_to_test(); // What --help does: "Test setup error: unknown type"
    }

    static void run(const std::string& enableArg)
    {
        test_tree_enabler enabler(enableArg);
        traverse_test_tree(framework::master_test_suite(), enabler);

        static gui_observer observer;
        framework::register_observer(observer);
    }

    static void wait()
    {
        std::cout << "#waiting" << std::endl;
        std::getchar();
    }

    static void traverse_test_tree(test_case const& tc, test_tree_visitor& v)
    {
        v.visit(tc);
    }

    static void traverse_test_tree(test_suite const& suite, test_tree_visitor& v)
    {
        if (!v.test_suite_start(suite))
            return;

        BOOST_TEST_FOREACH(test_unit_id, id, static_cast<const test_suite_access&>(suite).members())
        traverse_test_tree(id, v);

        v.test_suite_finish(suite);
    }

    static void traverse_test_tree(test_unit_id id, test_tree_visitor& v)
    {
        if (ut_detail::test_id_2_unit_type(id) == tut_case)
            traverse_test_tree(framework::get<test_case>(id), v);
        else
            traverse_test_tree(framework::get<test_suite>(id), v);
    }

private:
    boost::runtime::cla::parser m_args;
};

} // namespace gui

inline test_case* enable_test_case(test_case* tc, bool enable)
{
    tc->p_enabled.value = enable;
    return tc;
}

} // namespace unit_test
} // namespace boost


#define BOOST_FIXTURE_TEST_CASE_ENABLE(test_name, F, enable)    \
    struct test_name : public F                                 \
    {                                                           \
        void test_method();                                     \
    };                                                          \
                                                                \
    static void BOOST_AUTO_TC_INVOKER(test_name)()              \
    {                                                           \
        test_name t;                                            \
        t.test_method();                                        \
    }                                                           \
                                                                \
    struct BOOST_AUTO_TC_UNIQUE_ID(test_name)                   \
    {                                                           \
    };                                                          \
                                                                \
    BOOST_AUTO_TU_REGISTRAR(test_name)                          \
    (                                                           \
        boost::unit_test::enable_test_case(                     \
            boost::unit_test::make_test_case(                   \
                &BOOST_AUTO_TC_INVOKER(test_name), #test_name), \
            enable),                                            \
        boost::unit_test::ut_detail::auto_tc_exp_fail<          \
            BOOST_AUTO_TC_UNIQUE_ID(test_name)>::instance()     \
            ->value());                                         \
                                                                \
    void test_name::test_method()

#define BOOST_AUTO_TEST_CASE_ENABLE(test_name, enable) \
    BOOST_FIXTURE_TEST_CASE_ENABLE(test_name, BOOST_AUTO_TEST_CASE_FIXTURE, enable)

#ifndef BOOST_TEST_NO_GUI_INIT

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::gui::runner runner(argc, argv);
    boost::unit_test::test_suite* p = init_unit_test_suite2(argc, argv);
    runner();
    return p;
}

extern "C" __declspec(dllexport) inline void unit_test_type_boost()
{
}

#endif // !BOOST_TEST_NO_GUI_INIT

#endif // !BOOST_TEST_API_3

#define init_unit_test_suite init_unit_test_suite2

#endif // BOOST_TEST_UNIT_TEST_GUI_HPP
