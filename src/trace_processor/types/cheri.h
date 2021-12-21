//
// XXX add license header here

#ifndef SRC_TRACE_PROCESSOR_TYPES_CHERI_H_
#define SRC_TRACE_PROCESSOR_TYPES_CHERI_H_

namespace perfetto {
namespace trace_processor {

// Compartment identifier
// This is the internal key used to identify compartments. It is assumed that
// compartment identifiers may be reused. This acts as an Unix pid/tid for
// compartments from trace sources, and is tracked in a similar manner.
struct CompartmentId {
  uint64_t cid;
  base::Optional<uint32_t> el;

  bool operator==(const CompartmentId& other) const {
    return std::tie(cid, el) == std::tie(other.cid, other.el);
  }

  bool operator<(const CompartmentId& other) const {
    return std::tie(cid, el) < std::tie(other.cid, other.el);
  }
};

// A CHERI context is a generalised version of a process/thread track identifier.
// It is designed to be an unstructured key that does not impose a hierarchy among
// pid/tid and compartment IDs.
// Extra fields should be optional.
struct CHERIContextId {
  uint64_t pid;
  uint64_t tid;
  uint64_t cid;
  base::Optional<uint32_t> el;

  CompartmentId compartment_id() const {
    return {cid, el};
  }
};

}  // namespace trace_processor
}  // namespace perfetto

#endif  // SRC_TRACE_PROCESSOR_TYPES_CHERI_H_
