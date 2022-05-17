#ifndef SRC_TRACE_PROCESSOR_TABLES_INTERVAL_TABLES_H_
#define SRC_TRACE_PROCESSOR_TABLES_INTERVAL_TABLES_H_

#include "src/trace_processor/tables/macros.h"
#include "src/trace_processor/tables/track_tables.h"

namespace perfetto {
namespace trace_processor {
namespace tables {

// @tablegroup Events
// @param arg_set_id {@joinable args.arg_set_id}
#define PERFETTO_TP_INTERVAL_TABLE_DEF(NAME, PARENT, C)  \
  NAME(IntervalTable, "interval")                        \
      PERFETTO_TP_ROOT_TABLE(PARENT, C)                  \
      C(int64_t, ts, Column::Flag::kSorted)              \
      C(IntervalTrackTable::Id, track_id)                \
      C(int64_t, start)                                  \
      C(int64_t, end)                                    \
      C(int64_t, value)                                  \
      C(base::Optional<uint32_t>, arg_set_id)

PERFETTO_TP_TABLE(PERFETTO_TP_INTERVAL_TABLE_DEF);

} // namespace tables
} // namespace trace_processor
} // namespace perfetto

#endif // SRC_TRACE_PROCESSOR_TABLES_INTERVAL_TABLES_H_
