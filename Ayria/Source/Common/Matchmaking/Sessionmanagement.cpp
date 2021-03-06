/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-26
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Matchmaking
{
    std::vector<Session_t> LANSessions, WANSessions;
    Session_t Localsession{};

    // Manage the sessions we know of, updates in the background.
    std::vector<Session_t *> getLANSessions()
    {
        std::vector<Session_t *> Result;
        Result.reserve(LANSessions.size());
        for (auto it = LANSessions.begin(); it != LANSessions.end(); ++it)
            Result.push_back(&*it);
        return Result;
    }
    std::vector<Session_t *> getWANSessions()
    {
        std::vector<Session_t *> Result;
        Result.reserve(WANSessions.size());
        for (auto it = WANSessions.begin(); it != WANSessions.end(); ++it)
            Result.push_back(&*it);
        return Result;
    }
    Session_t *getLocalsession()
    {
        return &Localsession;
    }

    // Announce our session state once in a while.
    void __cdecl Sessionupdate()
    {
        const auto Currenttime = time(NULL);
        std::erase_if(LANSessions, [&](const auto &Session) { return (Session.Lastmessage + 15) < Currenttime; });
        std::erase_if(WANSessions, [&](const auto &Session) { return (Session.Lastmessage + 15) < Currenttime; });

        // TODO(tcn): Poll a server for a listing.

        if (Localsession.isActive) [[unlikely]]
        {
            auto Object = nlohmann::json::object({
                { "Hostlocale", Localsession.Hostinfo.Locale.asUTF8() },
                { "Hostname", Localsession.Hostinfo.Username.asUTF8() },
                { "HostID", Localsession.Hostinfo.ID.Raw },
                { "Sessiondata", Localsession.JSONData },
                { "Signature", Localsession.Signature }
            });
            Backend::Sendmessage(Hash::FNV1_32("Sessionupdate"), DumpJSON(Object), Backend::Matchmakeport);
        }
    }
    void __cdecl LANUpdatehandler(uint32_t NodeID, const char *JSONString)
    {
        const auto Client = Clientinfo::getNetworkclient(NodeID);
        if (!Client || Client->AccountID.Raw == 0) [[unlikely]]
            return; // WTF?

        const auto Publickey = Clientinfo::getPublickey(Client->AccountID.AccountID);
        if (Publickey.empty()) return;

        Session_t Session{ true, uint32_t(time(NULL)) };

        const auto Request = ParseJSON(JSONString);
        Session.Signature = Request.value("Signature", std::string());
        Session.Hostinfo.ID.Raw = Request.value("HostID", uint64_t());
        Session.JSONData = Request.value("Sessiondata", std::string());
        Session.Hostinfo.Locale = Request.value("Locale", std::u8string());
        Session.Hostinfo.Username = Request.value("Hostname", std::u8string());

        // Basic validation.
        if (Session.Hostinfo.ID.Raw == 0) return;
        if (Session.Signature.empty()) return;
        if (Session.JSONData.empty()) return;

        // Session-data has been modified or damaged.
        if (!PK_RSA::Verifysignature(Session.JSONData, Session.Signature, Base64::Decode(Publickey)))
            return;

        std::erase_if(LANSessions, [&](const auto &Item)
        {
            return Item.Hostinfo.ID.Raw == Session.Hostinfo.ID.Raw;
        });
        LANSessions.push_back(std::move(Session));
    }

    // Register handlers and set up session.
    void Initialize()
    {
        Backend::Enqueuetask(5000, Sessionupdate);
        Backend::Registermessagehandler(Hash::FNV1_32("Sessionupdate"), LANUpdatehandler);
    }
}
