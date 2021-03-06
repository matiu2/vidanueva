#include "App.hpp"

#include "MainPage.hpp"

#include <vidanueva/lib/page/view.hpp>
#include <vidanueva/lib/page/edit.hpp>
#include <vidanueva/mapTables.hpp>

#include <Wt/WText>
#include <Wt/Dbo/Exception>
#include <Wt/Auth/Identity>
#include <Wt/Auth/AuthWidget>
#include <Wt/WDialog>

#include <stdexcept>

namespace vidanueva {

namespace dbo = Wt::Dbo;

App::App(const Wt::WEnvironment& env, dbo::SqlConnection& db, const Auth::Services& services)
    : Wt::WApplication(env), auth(_session, services)
{
    // Configure our messages
    messageResourceBundle().use(appRoot() + "/page/view");
    messageResourceBundle().use(appRoot() + "/app");
    // Set up the DB session
    _session.setConnection(db);
    mapTables(_session);
    try {
        log("info") << "Creating tables";
        _session.createTables();
        _session.execute("CREATE UNIQUE INDEX CONCURRENTLY page_name ON page (name)");
        _session.execute("ALTER TABLE page ALTER COLUMN title SET NOT NULL");
        _session.execute("ALTER TABLE page ALTER COLUMN name SET NOT NULL");
        _session.execute("ALTER TABLE page ALTER COLUMN body SET NOT NULL");
    } catch ( std::exception e ) {
        log("info") << "Table creation failed: " << e.what();
    } catch (...) {
        log("info") << "Table creation failed. Exception not caught.";
    }
    dbo::Transaction t(_session);
    // Create a welcome page if one doesn't already exist
    page::pModel welcome = _session.find<page::Model>().where("name = ?").bind("index");
    if (!welcome) {
        log("info") << "Creating welcome page";
        welcome = _session.add(new page::Model{"index", "Welcome", "To Vidanueva"});
    }
    // Create the admin user if one doesn't exist
    auto wtUser = auth.users().findWithIdentity(Wt::Auth::Identity::LoginName, "admin");
    if (!wtUser.isValid()) {
        log("info") << "Creating admin user";
        auto wtUser = auth.users().registerNew();
        wtUser.addIdentity(Wt::Auth::Identity::LoginName, "admin");
        auth.services.pword.updatePassword(wtUser, "admin");
        Auth::pUser user = _session.add(new Auth::User{"Mister Admin", true});
        auth.users().find(wtUser).modify()->setUser(user);
    }
    t.commit();
    // Load the style sheets
    setCssTheme("polished");
    useStyleSheet("/css/fonts.css");
    useStyleSheet("/css/layout.css");
    useStyleSheet("/css/colour.css");
    // Show the main page
    mainPage = new MainPage(root());
    internalPathChanged().connect(this, &App::pathChanged);
    pathChanged(internalPath());
}

void App::pathChanged(std::string path) {
    showLogin();
    dbo::Transaction t(_session);
    page::pModel welcome = _session.find<page::Model>().where("name = ?").bind("index");
    if (auth.isAdmin())
        mainPage->setBody(new page::AdminWebView(welcome));
    else
        mainPage->setBody(new page::WebView(welcome));
    // TODO: look up pages and sermons here
}

void App::showLogin() {
    log("info") << "Showing dialog";
    auto loginForm = new Wt::Auth::AuthWidget(auth.services.auth, auth.users(), auth.login(), root());
    loginForm->setRegistrationEnabled(false);
    loginForm->model()->addPasswordAuth(&auth.services.pword);
    loginForm->processEnvironment();
}


}
