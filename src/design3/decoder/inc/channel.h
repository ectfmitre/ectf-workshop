#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include <vector>

#include "keys.h"
#include "types.h"

namespace ectf {

constexpr int MAX_CHANNELS = 9;

// Container for channel-related data, including subscription information
// and encryptions keys if applicable.
class Channel {
private:
	ChannelID channel_id_;
	PageNumber flash_page_num_;
	// whether we have an active subscription
	bool active_ = false;
	// start and end times for the subscription
	Timestamp start_time_;
	Timestamp end_time_;
	// channel keys
	EdPublicKey public_key_;
	ChaChaKey symmetric_key_;

	Channel() {}
	~Channel() {}
	void Init(ChannelID channel_id, PageNumber page_num) {
		channel_id_ = channel_id;
		flash_page_num_ = page_num;
	}
	friend class ChannelData;
public:
	Channel& operator=(const Channel& other) = default;
	ChannelID GetID() const { return channel_id_; }
	PageNumber GetFlashPageNumber() const { return flash_page_num_; }
	bool IsActive() const { return active_; }
	Timestamp GetStartTime() const { return start_time_; }
	Timestamp GetEndTime() const { return end_time_; }
	const EdPublicKey& GetPublicKey() const { return public_key_; }
	const ChaChaKey& GetSymmetricKey() const { return symmetric_key_; }
	// Marks the channel as having an expired/inactive subscription.
	void ClearSubscription();
	// Loads an active subscription.
	void SetSubscription(Timestamp start_time, Timestamp end_time,
			EdPublicKey public_key, ChaChaKey symmetric_key);
};

// Container for data related to all channels.
class ChannelData {
private:
	Channel channels_[MAX_CHANNELS];
	// The number of populated entries in the channels array.
	int num_channels_;
	// The value of the largest timestamp ever seen in a decoded frame.
	Timestamp last_seen_time_ = 0;
public:
	ChannelData();
	// Returns a list of all flash page numbers that could possibly be used to
	// store a subscription for a channel.
	std::vector<PageNumber> GetAllFlashPageNumbers();
	// Returns all known channels. Pointers will not be null.
	std::vector<Channel*> GetAllChannels();
	// Returns all channels except the broadcast (channel 0). Pointers will not
	// be null.
	std::vector<Channel*> GetNonZeroChannels();
	// Returns the channel with the given ID, or a null pointer if there is
	// no such channel.
	Channel* GetChannel(ChannelID channel_id);
	// Returns the channel with the given ID, creating a new channel object
	// if necessary. Can return a null pointer if the maximuum number of
	// channels has already been reached.
	Channel* GetOrCreateChannel(ChannelID channel_id);
	// Returns the largest decoded frame timestamp.
	Timestamp GetLastSeenTime() const { return last_seen_time_; }
	// Stores the largest decoded frame timestamp.
	void SetLastSeenTime(Timestamp time) { last_seen_time_ = time; }
};

}

#endif // __CHANNEL_H__
