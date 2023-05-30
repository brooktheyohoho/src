#ifndef _ANTICHEAT_MANAGER_H_
#define _ANTICHEAT_MANAGER_H_
#include <vector>

enum EAnticheatScanIDs
{
	NET_SCAN_ID_NULL,
	NET_SCAN_ID_PROCESS,
	NET_SCAN_ID_MAX
};

class CAnticheatManager : public singleton <CAnticheatManager>
{
	public:
		bool Initialize();
		void Release();

		bool CreateClientHandle(LPCHARACTER ch);
		void DeleteClientHandle(DWORD dwPlayerID);

		bool SendBlacklistPacket(LPCHARACTER ch);
		bool SendCheckPacket(LPCHARACTER ch);

		bool ReloadCheatBlacklists();

	protected:
		bool __ParseCheatBlacklist(DWORD uiScanID, const std::string & szTableName);

	private:
		std::vector <std::string> m_vSerializedBlacklists;
};

#endif /* _ANTICHEAT_MANAGER_H_ */

