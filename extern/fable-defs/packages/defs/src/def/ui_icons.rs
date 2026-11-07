use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct UiIconsDef {
    #[def("IconFriendRequestReceived")]
    pub icon_friend_request_received: u32,
    #[def("IconFriendRequestReceivedOn")]
    pub icon_friend_request_received_on: u32,
    #[def("IconFriendRequestSent")]
    pub icon_friend_request_sent: u32,
    #[def("IconFriendRequestSentOn")]
    pub icon_friend_request_sent_on: u32,
    #[def("IconGameInviteReceived")]
    pub icon_game_invite_received: u32,
    #[def("IconGameInviteReceivedOn")]
    pub icon_game_invite_received_on: u32,
    #[def("IconGameInviteSent")]
    pub icon_game_invite_sent: u32,
    #[def("IconGameInviteSentOn")]
    pub icon_game_invite_sent_on: u32,
    #[def("IconMute")]
    pub icon_mute: u32,
    #[def("IconMuteOn")]
    pub icon_mute_on: u32,
    #[def("IconOnline")]
    pub icon_online: u32,
    #[def("IconOnlineOn")]
    pub icon_online_on: u32,
    #[def("IconPasscodeBlank")]
    pub icon_passcode_blank: u32,
    #[def("IconPasscodeFilled")]
    pub icon_passcode_filled: u32,
    #[def("IconTV")]
    pub icon_tv: u32,
    #[def("IconTVOn")]
    pub icon_tv_on: u32,
    #[def("IconVoice")]
    pub icon_voice: u32,
    #[def("IconVoiceOn")]
    pub icon_voice_on: u32,
    #[def("IconWait1")]
    pub icon_wait1: u32,
    #[def("IconWait2")]
    pub icon_wait2: u32,
    #[def("IconWait3")]
    pub icon_wait3: u32,
    #[def("IconWait4")]
    pub icon_wait4: u32,
    #[def("IconProgress")]
    pub icon_progress: u32,
    #[def("IconProgressOn")]
    pub icon_progress_on: u32,
    #[def("IconA")]
    pub icon_a: u32,
    #[def("IconB")]
    pub icon_b: u32,
    #[def("IconX")]
    pub icon_x: u32,
    #[def("IconY")]
    pub icon_y: u32,
    #[def("IconBlank")]
    pub icon_blank: u32,
    #[def("IconUpArrow")]
    pub icon_up_arrow: u32,
    #[def("IconDownArrow")]
    pub icon_down_arrow: u32,
    #[def("IconListHighlight")]
    pub icon_list_highlight: u32,
}
