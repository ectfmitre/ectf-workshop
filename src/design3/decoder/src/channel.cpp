#include "channel.h"

#include <vector>

#include "debug.h"
#include "keys.h"
#include "types.h"

namespace ectf {

void Channel::ClearSubscription() {
	active_ = false;
	public_key_.Clear();
	symmetric_key_.Clear();
}

void Channel::SetSubscription(Timestamp start_time, Timestamp end_time,
		EdPublicKey public_key, ChaChaKey symmetric_key) {
	active_ = true;
	start_time_ = start_time;
	end_time_ = end_time;
	public_key_ = public_key;
	symmetric_key_ = symmetric_key;
}

ChannelData::ChannelData() {
	num_channels_ = 1;
	channels_[0].Init(0, 0);
}

std::vector<PageNumber> ChannelData::GetAllFlashPageNumbers() {
	std::vector<PageNumber> ret;
	for (PageNumber p = 1; p < MAX_CHANNELS; p++) {
		ret.push_back(p);
	}
	return ret;
}

std::vector<Channel*> ChannelData::GetAllChannels() {
	std::vector<Channel*> ret;
	for (int i = 0; i < num_channels_; i++) {
		ret.push_back(&channels_[i]);
	}
	return ret;
}

std::vector<Channel*> ChannelData::GetNonZeroChannels() {
	std::vector<Channel*> ret;
	for (int i = 0; i < num_channels_; i++) {
		Channel& channel = channels_[i];
		if (channel.GetID() != 0) {
			ret.push_back(&channel);
		}
	}
	return ret;
}

Channel* ChannelData::GetChannel(ChannelID channel_id) {
	for (int i = 0; i < num_channels_; i++) {
		if (channels_[i].GetID() == channel_id) {
			return &channels_[i];
		}
	}
	return nullptr;
}

Channel* ChannelData::GetOrCreateChannel(ChannelID channel_id) {
	Channel* channel = GetChannel(channel_id);
	if (channel) return channel;
	if (num_channels_ < MAX_CHANNELS) {
		channels_[num_channels_].Init(channel_id, num_channels_);
		num_channels_++;
		return &channels_[num_channels_ - 1];
	}
	return nullptr;
}

}  // namespace ectf
