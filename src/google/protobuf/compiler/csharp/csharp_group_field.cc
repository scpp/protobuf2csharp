// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <sstream>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/wire_format_lite.h>

#include <google/protobuf/compiler/csharp/csharp_doc_comment.h>
#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/compiler/csharp/csharp_group_field.h>
#include <google/protobuf/compiler/csharp/csharp_options.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

GroupFieldGenerator::GroupFieldGenerator(const FieldDescriptor* descriptor,
                                             const Options *options)
    : MessageFieldGenerator(descriptor, options) {
  int tag_size = internal::WireFormat::TagSize(descriptor_->number(), descriptor_->type()) / 2;
  uint tag = internal::WireFormatLite::MakeTag(
      descriptor_->number(),
      internal::WireFormatLite::WIRETYPE_END_GROUP);
  uint8 tag_array[5];
  io::CodedOutputStream::WriteTagToArray(tag, tag_array);
  string tag_bytes = SimpleItoa(tag_array[0]);
  for (int i = 1; i < tag_size; i++) {
      tag_bytes += ", " + SimpleItoa(tag_array[i]);
  }
  variables_["end_tag"] = SimpleItoa(tag);
  variables_["end_tag_bytes"] = tag_bytes;
}

GroupFieldGenerator::~GroupFieldGenerator() {

}

void GroupFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_not_property_check$) {\n"
    "  $name$_ = new $type_name$();\n"
    "}\n"
    "input.ReadGroup($name$_);\n");
}

void GroupFieldGenerator::GenerateSerializationCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_property_check$) {\n" 
    "  output.WriteRawTag($tag_bytes$);\n"
    "  output.WriteGroup($property_name$);\n"
    "  output.WriteRawTag($end_tag_bytes$);\n"
    "}\n");
}

void GroupFieldGenerator::GenerateSerializedSizeCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_property_check$) {\n"
    "  size += $tag_size$ + $tag_size$ + pb::CodedOutputStream.ComputeGroupSize($property_name$);\n"
    "}\n");
}

void GroupFieldGenerator::GenerateCodecCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "pb::FieldCodec.ForGroup($tag$, $end_tag$, $type_name$.Parser)");
}

GroupOneofFieldGenerator::GroupOneofFieldGenerator(
    const FieldDescriptor* descriptor,
    const Options *options)
    : GroupFieldGenerator(descriptor, options) {
  SetCommonOneofFieldVariables(&variables_);
}

GroupOneofFieldGenerator::~GroupOneofFieldGenerator() {

}

void GroupOneofFieldGenerator::GenerateMembers(io::Printer* printer) {
  WritePropertyDocComment(printer, descriptor_);
  AddPublicMemberAttributes(printer);
  printer->Print(
    variables_,
    "$access_level$ $type_name$ $property_name$ {\n"
    "  get { return $has_property_check$ ? ($type_name$) $oneof_name$_ : null; }\n"
    "  set {\n"
    "    $oneof_name$_ = value;\n"
    "    $oneof_name$Case_ = value == null ? $oneof_property_name$OneofCase.None : $oneof_property_name$OneofCase.$property_name$;\n"
    "  }\n"
    "}\n");
  printer->Print(
      variables_,
      "/// <summary>Gets whether the \"$descriptor_name$\" field is set</summary>\n");
    AddPublicMemberAttributes(printer);
    printer->Print(
      variables_,
      "$access_level$ bool Has$property_name$ {\n"
      "  get { return $oneof_name$Case_ == $oneof_property_name$OneofCase.$property_name$; }\n"
      "}\n");
  printer->Print(
    variables_,
    "/// <summary> Clears the value of the oneof if it's currently set to \"$descriptor_name$\" </summary>\n");
  AddPublicMemberAttributes(printer);
  printer->Print(
    variables_,
    "$access_level$ void Clear$property_name$() {\n"
    "  if ($has_property_check$) {\n"
    "    Clear$oneof_property_name$();\n"
    "  }\n"
    "}\n");
}

void GroupOneofFieldGenerator::GenerateMergingCode(io::Printer* printer) {
  printer->Print(variables_, 
    "if ($property_name$ == null) {\n"
    "  $property_name$ = new $type_name$();\n"
    "}\n"
    "$property_name$.MergeFrom(other.$property_name$);\n");
}

void GroupOneofFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  // TODO(jonskeet): We may be able to do better than this
  printer->Print(
    variables_,
    "$type_name$ subBuilder = new $type_name$();\n"
    "if ($has_property_check$) {\n"
    "  subBuilder.MergeFrom($property_name$);\n"
    "}\n"
    "input.ReadGroup(subBuilder);\n"
    "$property_name$ = subBuilder;\n");
}

void GroupOneofFieldGenerator::WriteToString(io::Printer* printer) {
  printer->Print(
    variables_,
    "PrintField(\"$descriptor_name$\", $has_property_check$, $oneof_name$_, writer);\n");
}

void GroupOneofFieldGenerator::GenerateCloningCode(io::Printer* printer) {
  printer->Print(variables_,
    "$property_name$ = other.$property_name$.Clone();\n");
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
