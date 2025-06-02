#ifndef THIRD_PARTY_ODML_LITERT_LM_SCHEMA_CORE_LITERTLM_EXPORT_H_
#define THIRD_PARTY_ODML_LITERT_LM_SCHEMA_CORE_LITERTLM_EXPORT_H_

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "absl/status/status.h"  // from @com_google_absl
#include "flatbuffers/buffer.h"  // from @flatbuffers
#include "flatbuffers/flatbuffer_builder.h"  // from @flatbuffers
#include "schema/core/litertlm_header.h"
#include "schema/core/litertlm_header_schema_generated.h"
#include "schema/core/litertlm_section.h"

namespace litert {

namespace lm {

namespace schema {

using KVPair = ::flatbuffers::Offset<KeyValuePair>;
using KVPairs = std::vector<KVPair>;

// Make a LiteRT-LM file from sections.
//
// Args:
//   builder: flatbuffer builder for the header.
//   sections: a vector of section stream base.
//   section_types: a vector of section data type.
//   system_metadata_map: a vector of system metadata key value pair.
//   section_items_maps: a vector of key-value pairs for section metadata.
//   out_path: output path of the LiteRT-LM file.
//
// Returns:
//   absl::Status.
absl::Status MakeLiteRTLMFromSections(
    flatbuffers::FlatBufferBuilder& builder,
    const std::vector<std::unique_ptr<SectionStreamBase>>& sections,
    const std::vector<AnySectionDataType>& section_types,
    const std::vector<KVPair>& system_metadata_map,
    const std::vector<std::vector<KVPair>>& section_items_maps,
    const std::string& out_path);

}  // end namespace schema
}  // end namespace lm
}  // end namespace litert

#endif  // THIRD_PARTY_ODML_LITERT_LM_SCHEMA_CORE_LITERTLM_EXPORT_H_
