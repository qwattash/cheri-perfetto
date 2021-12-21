/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SRC_TRACE_PROCESSOR_IMPORTERS_PROTO_TRACK_EVENT_TRACKER_H_
#define SRC_TRACE_PROCESSOR_IMPORTERS_PROTO_TRACK_EVENT_TRACKER_H_

#include "src/trace_processor/storage/trace_storage.h"
#include "src/trace_processor/types/trace_processor_context.h"
#include "src/trace_processor/types/cheri.h"

namespace perfetto {
namespace trace_processor {

// Tracks and stores tracks based on track types, ids and scopes.
class TrackEventTracker {
 public:
  explicit TrackEventTracker(TraceProcessorContext*);

  // Associate a TrackDescriptor track identified by the given |uuid| with a
  // process's |pid|. This is called during tokenization. If a reservation for
  // the same |uuid| already exists, verifies that the present reservation
  // matches the new one.
  //
  // The track will be resolved to the process track (see InternProcessTrack())
  // upon the first call to GetDescriptorTrack() with the same |uuid|. At this
  // time, |pid| will also be resolved to a |upid|.
  void ReserveDescriptorProcessTrack(uint64_t uuid,
                                     StringId name,
                                     uint32_t pid,
                                     int64_t timestamp);

  // Associate a TrackDescriptor track identified by the given |uuid| with a
  // thread's |pid| and |tid|. This is called during tokenization. If a
  // reservation for the same |uuid| already exists, verifies that the present
  // reservation matches the new one.
  //
  // The track will be resolved to the thread track (see InternThreadTrack())
  // upon the first call to GetDescriptorTrack() with the same |uuid|. At this
  // time, |pid| will also be resolved to a |upid|.
  void ReserveDescriptorThreadTrack(uint64_t uuid,
                                    uint64_t parent_uuid,
                                    StringId name,
                                    uint32_t pid,
                                    uint32_t tid,
                                    int64_t timestamp);

  // Associate a TrackDescriptor track identified by the given |uuid| with a
  // CHERI context. This is called during tokenization.
  // If a reservation for the same |uuid| already exists, verifies that the present
  // reservation matches the new one.
  //
  // The track will be resolved to the CHERI context track
  // (see InternCHERIContextTrack()) upon the first call to GetDescriptorTrack() with
  // the same |uuid|. At this time, the identifiers |pid|, |tid| and |cid| will be
  // resolved to unique identifiers.
  void ReserveDescriptorCHERIContextTrack(uint64_t uuid,
                                          uint64_t parent_uuid,
                                          StringId name,
                                          CHERIContextId ccid,
                                          int64_t timestamp);

  // Associate a TrackDescriptor track identified by the given |uuid| with a
  // parent track (usually a process- or thread-associated track). This is
  // called during tokenization. If a reservation for the same |uuid| already
  // exists, will attempt to update it.
  //
  // The track will be created upon the first call to GetDescriptorTrack() with
  // the same |uuid|. If |parent_uuid| is 0, the track will become a global
  // track. Otherwise, it will become a new track of the same type as its parent
  // track.
  void ReserveDescriptorChildTrack(uint64_t uuid,
                                   uint64_t parent_uuid,
                                   StringId name);

  // Associate a counter-type TrackDescriptor track identified by the given
  // |uuid| with a parent track (usually a process or thread track). This is
  // called during tokenization. If a reservation for the same |uuid| already
  // exists, will attempt to update it. The provided |category| will be stored
  // into the track's args.
  //
  // If |is_incremental| is true, the counter will only be valid on the packet
  // sequence identified by |packet_sequence_id|. |unit_multiplier| is an
  // optional multiplication factor applied to counter values. Values for the
  // counter will be translated during tokenization via
  // ConvertToAbsoluteCounterValue().
  //
  // The track will be created upon the first call to GetDescriptorTrack() with
  // the same |uuid|. If |parent_uuid| is 0, the track will become a global
  // track. Otherwise, it will become a new counter track for the same
  // process/thread as its parent track.
  void ReserveDescriptorCounterTrack(uint64_t uuid,
                                     uint64_t parent_uuid,
                                     StringId name,
                                     StringId category,
                                     int64_t unit_multiplier,
                                     bool is_incremental,
                                     uint32_t packet_sequence_id);

  // Returns the ID of the track for the TrackDescriptor with the given |uuid|.
  // This is called during parsing. The first call to GetDescriptorTrack() for
  // each |uuid| resolves and inserts the track (and its parent tracks,
  // following the parent_uuid chain recursively) based on reservations made for
  // the |uuid|. If the track is a child track and doesn't have a name yet,
  // updates the track's name to event_name. Returns nullopt if no track for a
  // descriptor with this |uuid| has been reserved.
  // TODO(lalitm): this method needs to be split up and moved back to
  // TrackTracker.
  base::Optional<TrackId> GetDescriptorTrack(
      uint64_t uuid,
      StringId event_name = kNullStringId);

  // Converts the given counter value to an absolute value in the unit of the
  // counter, applying incremental delta encoding or unit multipliers as
  // necessary. If the counter uses incremental encoding, |packet_sequence_id|
  // must match the one in its track reservation. Returns base::nullopt if the
  // counter track is unknown or an invalid |packet_sequence_id| was passed.
  base::Optional<double> ConvertToAbsoluteCounterValue(
      uint64_t counter_track_uuid,
      uint32_t packet_sequence_id,
      double value);

  // Returns the ID of the implicit trace-global default TrackDescriptor track.
  // TODO(lalitm): this method needs to be moved back to TrackTracker once
  // GetDescriptorTrack is moved back.
  TrackId GetOrCreateDefaultDescriptorTrack();

  // Called by ProtoTraceReader whenever incremental state is cleared on a
  // packet sequence. Resets counter values for any incremental counters of
  // the sequence identified by |packet_sequence_id|.
  void OnIncrementalStateCleared(uint32_t packet_sequence_id);

 private:
  struct DescriptorTrackReservation {
    uint64_t parent_uuid = 0;
    base::Optional<uint32_t> pid;
    base::Optional<uint32_t> tid;
    base::Optional<CHERIContextId> cheri_context;
    int64_t min_timestamp = 0;  // only set if |pid| and/or |tid| is set.
    StringId name = kNullStringId;

    // For counter tracks.
    bool is_counter = false;
    StringId category = kNullStringId;
    int64_t unit_multiplier = 1;
    bool is_incremental = false;
    uint32_t packet_sequence_id = 0;
    double latest_value = 0;

    // Whether |other| is a valid descriptor for this track reservation. A track
    // should always remain nested underneath its original parent.
    bool IsForSameTrack(const DescriptorTrackReservation& other) {
      // Note that |min_timestamp|, |latest_value|, and |name| are ignored for
      // this comparison.
      return std::tie(parent_uuid, pid, tid, is_counter, category,
                      unit_multiplier, is_incremental, packet_sequence_id) ==
             std::tie(other.parent_uuid, pid, tid, is_counter, category,
                      unit_multiplier, is_incremental, packet_sequence_id);
    }
  };

  class ResolvedDescriptorTrack {
   public:
    enum class Scope {
      kThread,
      kProcess,
      kGlobal,
      kCHERI,
    };

    static ResolvedDescriptorTrack Process(UniquePid upid,
                                           bool is_counter,
                                           bool is_root);
    static ResolvedDescriptorTrack Thread(UniqueTid utid,
                                          bool is_counter,
                                          bool is_root);
    static ResolvedDescriptorTrack CHERIContext(UniquePid upid,
                                                UniqueTid utid,
                                                UniqueCid ucid,
                                                bool is_counter,
                                                bool is_root);
    static ResolvedDescriptorTrack Global(bool is_counter, bool is_root);

    Scope scope() const { return scope_; }
    bool is_counter() const { return is_counter_; }
    UniqueTid utid() const {
      PERFETTO_DCHECK(scope() == Scope::kThread || scope() == Scope::kCHERI);
      return utid_;
    }
    UniquePid upid() const {
      PERFETTO_DCHECK(scope() == Scope::kProcess || scope() == Scope::kCHERI);
      return upid_;
    }
    UniqueCid ucid() const {
      PERFETTO_DCHECK(scope() == Scope::kCHERI);
      return ucid_;
    }
    UniqueTid is_root_in_scope() const { return is_root_in_scope_; }

   private:
    Scope scope_;
    bool is_counter_;
    bool is_root_in_scope_;

    // Only set when |scope| == |Scope::kThread|.
    UniqueTid utid_;

    // Only set when |scope| == |Scope::kProcess|.
    UniquePid upid_;

    // Only set when |scope| == |Scope::kCHERI|.
    UniqueCid ucid_;
  };

  base::Optional<TrackId> GetDescriptorTrackImpl(uint64_t uuid);
  TrackId CreateTrackFromResolved(const ResolvedDescriptorTrack&);
  base::Optional<ResolvedDescriptorTrack> ResolveDescriptorTrack(
      uint64_t uuid,
      std::vector<uint64_t>* descendent_uuids);
  ResolvedDescriptorTrack ResolveDescriptorTrackImpl(
      uint64_t uuid,
      const DescriptorTrackReservation&,
      std::vector<uint64_t>* descendent_uuids);

  static constexpr uint64_t kDefaultDescriptorTrackUuid = 0u;

  std::map<UniqueTid, TrackId> thread_tracks_;
  std::map<UniquePid, TrackId> process_tracks_;

  std::map<uint64_t /* uuid */, DescriptorTrackReservation>
      reserved_descriptor_tracks_;
  std::map<uint64_t /* uuid */, ResolvedDescriptorTrack>
      resolved_descriptor_tracks_;
  std::map<uint64_t /* uuid */, TrackId> descriptor_tracks_;

  // Stores the descriptor uuid used for the primary process/thread track
  // for the given upid / utid. Used for pid/tid reuse detection.
  std::map<UniquePid, uint64_t /*uuid*/> descriptor_uuids_by_upid_;
  std::map<UniqueTid, uint64_t /*uuid*/> descriptor_uuids_by_utid_;

  const StringId source_key_ = kNullStringId;
  const StringId source_id_key_ = kNullStringId;
  const StringId is_root_in_scope_key_ = kNullStringId;
  const StringId category_key_ = kNullStringId;

  const StringId descriptor_source_ = kNullStringId;

  const StringId default_descriptor_track_name_ = kNullStringId;

  TraceProcessorContext* const context_;
};

}  // namespace trace_processor
}  // namespace perfetto

#endif  // SRC_TRACE_PROCESSOR_IMPORTERS_PROTO_TRACK_EVENT_TRACKER_H_
