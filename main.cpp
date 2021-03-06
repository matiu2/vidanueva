#include <vidanueva/app/App.hpp>
#include <vidanueva/auth/Services.hpp>
#include <vidanueva/postgres.hpp>

int main(int argc, char** argv) {
    Wt::Dbo::backend::Postgres pg(pg_string);
    const Auth::Services authServices;
    Wt::WRun(argc, argv, [&](const Wt::WEnvironment& env) { return new vidanueva::App(env, pg, authServices); });
}
