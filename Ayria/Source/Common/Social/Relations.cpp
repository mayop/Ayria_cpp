/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-26
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Social
{
    namespace Relations
    {
        std::vector<Relation_t> Relations;

        void Add(uint32_t UserID, std::u8string_view Username, uint32_t Relationflags)
        {
            // Update existing.
            for (auto &Item : Relations)
            {
                if (Item.AccountID == UserID)
                {
                    Item.Flags |= Relationflags;

                    if (!Username.empty())
                        Item.Username = Username;

                    Save();
                    return;
                }
            }

            Relations.push_back({ UserID, std::u8string(Username), Relationflags });
            Save();
        }
        void Remove(uint32_t UserID, std::u8string_view Username)
        {
            std::erase_if(Relations, [&](const auto &Item)
            {
                return (UserID == Item.AccountID) || (Username == Item.Username);
            });
            Save();
        }
        const std::vector<Relation_t> *Get()
        {
            return &Relations;
        }

        // Disk management.
        void Load(std::wstring_view Path)
        {
            const auto Filebuffer = FS::Readfile(Path);
            if (Filebuffer.empty()) return;

            const auto Array = ParseJSON((char *)Filebuffer.c_str());
            if (!Array.is_array())
            {
                Warningprint("Relations.json is malformed, check FAQ or delete it.");
                return;
            }

            for (const auto &Object : Array)
            {
                Relation_t Relation;
                Relation.Flags = Object.value("Flags", uint32_t());
                Relation.AccountID = Object.value("UserID", uint32_t());
                Relation.Username = Object.value("Username", std::u8string());
                if (Relation.AccountID != 0) Relations.push_back(std::move(Relation));
            }
        }
        void Save(std::wstring_view Path)
        {
            auto Array = nlohmann::json::array();
            for (const auto &[ID, Username, Flags] : *Relations::Get())
            {
                const Relationflags_t Internal{ Flags };
                if (Internal.isFriend)
                {
                    Array += { { "Username", Username }, { "UserID", ID }, { "Flags", Flags }};
                }
            }

            FS::Writefile(Path,
                "// Auto-generated by Ayria, remember to escape UTF8 chars..\n"s +
                "Format: [ { Username, UserID, Flags }, ...] \n"s +
                Array.dump(4, ' ', true));
        }
    }
}
