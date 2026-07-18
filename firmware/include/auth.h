#pragma once

#include <WebServer.h>

void authBegin();
bool authIsLoggedIn(WebServer& server);
void authIssueSession(WebServer& server);
void authClearSession(WebServer& server);
bool authCheckCredentials(const String& email, const String& password);

// Returns true if authenticated; otherwise sends 401 JSON and returns false.
bool requireAuth(WebServer& server);
