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

#include <google/protobuf/compiler/csharp/csharp_doc_comment.h>
#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/compiler/csharp/csharp_options.h>
#include <google/protobuf/compiler/csharp/csharp_primitive_field.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

PrimitiveFieldGenerator::PrimitiveFieldGenerator(
    const FieldDescriptor* descriptor, const Options *options)
    : FieldGeneratorBase(descriptor, options) {
  // TODO(jonskeet): Make this cleaner...
  is_value_type = descriptor->type() != FieldDescriptor::TYPE_STRING
      && descriptor->type() != FieldDescriptor::TYPE_BYTES;
}

PrimitiveFieldGenerator::~PrimitiveFieldGenerator() {
}

void PrimitiveFieldGenerator::GenerateMembers(io::Printer* printer) {
  // TODO(jonskeet): Work out whether we want to prevent the fields from ever being
  // null, or whether we just handle it, in the cases of bytes and string.
  // (Basically, should null-handling code be in the getter or the setter?)

  printer->Print(variables_, "/// <summary>Default value for the \"$descriptor_name$\" field</summary>\n");
  printer->Print(variables_, "$access_level$ ");
  if (descriptor_->type() == FieldDescriptor::TYPE_BYTES) {
    printer->Print("readonly static ");
  }
  else {
    printer->Print("const ");
  }
  printer->Print(
    variables_,
    "$type_name$ $property_name$DefaultValue = $default_value$;\n\n");
  printer->Print(
              variables_,
              "private $nullable_type_name$ $name_def_message$;\n");
  WritePropertyDocComment(printer, descriptor_);
  AddPublicMemberAttributes(printer);
  printer->Print(
      variables_,
      "$access_level$ $type_name$ $property_name$ {\n"
      "  get { return $name$_ ?? $property_name$DefaultValue; }\n"
      "  set {\n");
  
  if (is_value_type) {
    printer->Print(
      variables_,
      "    $name$_ = value;\n");
  } else {
    printer->Print(
      variables_,
      "    $name$_ = pb::ProtoPreconditions.CheckNotNull(value, \"value\");\n");
  }
  printer->Print(
    "  }\n"
    "}\n");
  printer->Print(variables_, "/// <summary>Gets whether the \"$descriptor_name$\" field is set</summary>\n");
    AddPublicMemberAttributes(printer);
    printer->Print(
      variables_, 
      "$access_level$ bool Has$property_name$ {\n"
      "  get { return $name$_ != null; }\n"
      "}\n");
  printer->Print(variables_, "/// <summary>Clears the value of the \"$descriptor_name$\" field</summary>\n");
  AddPublicMemberAttributes(printer);
  printer->Print(
    variables_,
    "$access_level$ void Clear$property_name$() {\n");
  printer->Print(variables_, "  $name$_ = null;\n");
  printer->Print("}\n");
}

void PrimitiveFieldGenerator::GenerateMergingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($other_has_property_check$) {\n"
    "  $property_name$ = other.$property_name$;\n"
    "}\n");
}

void PrimitiveFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  // Note: invoke the property setter rather than writing straight to the field,
  // so that we can normalize "null to empty" for strings and bytes.
  printer->Print(
    variables_,
    "$property_name$ = input.Read$capitalized_type_name$();\n");
}

void PrimitiveFieldGenerator::GenerateSerializationCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_property_check$) {\n"
    "  output.WriteRawTag($tag_bytes$);\n"
    "  output.Write$capitalized_type_name$($property_name$);\n"
    "}\n");
}

void PrimitiveFieldGenerator::GenerateSerializedSizeCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_property_check$) {\n");
  printer->Indent();
  int fixedSize = GetFixedSize(descriptor_->type());
  if (fixedSize == -1) {
    printer->Print(
      variables_,
      "size += $tag_size$ + pb::CodedOutputStream.Compute$capitalized_type_name$Size($property_name$);\n");
  } else {
    printer->Print(
      "size += $tag_size$ + $fixed_size$;\n",
      "fixed_size", SimpleItoa(fixedSize),
      "tag_size", variables_["tag_size"]);
  }
  printer->Outdent();
  printer->Print("}\n");
}

void PrimitiveFieldGenerator::WriteHash(io::Printer* printer) {
  const char *text = "if ($has_property_check$) hash ^= $property_name$.GetHashCode();\n";
  if (descriptor_->type() == FieldDescriptor::TYPE_FLOAT) {
    text = "if ($has_property_check$) hash ^= pbc::ProtobufEqualityComparers.BitwiseSingleEqualityComparer.GetHashCode($property_name$);\n";
  } else if (descriptor_->type() == FieldDescriptor::TYPE_DOUBLE) {
    text = "if ($has_property_check$) hash ^= pbc::ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.GetHashCode($property_name$);\n";
  }
	printer->Print(variables_, text);
}
void PrimitiveFieldGenerator::WriteEquals(io::Printer* printer) {
  const char *text = "if ($property_name$ != other.$property_name$) return false;\n";
  if (descriptor_->type() == FieldDescriptor::TYPE_FLOAT) {
    text = "if (!pbc::ProtobufEqualityComparers.BitwiseSingleEqualityComparer.Equals($property_name$, other.$property_name$)) return false;\n";
  } else if (descriptor_->type() == FieldDescriptor::TYPE_DOUBLE) {
    text = "if (!pbc::ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.Equals($property_name$, other.$property_name$)) return false;\n";
  }
  printer->Print(variables_, text);
}
void PrimitiveFieldGenerator::WriteToString(io::Printer* printer) {
  printer->Print(
    variables_,
    "PrintField(\"$descriptor_name$\", $has_property_check$, $property_name$, writer);\n");
}

void PrimitiveFieldGenerator::GenerateCloningCode(io::Printer* printer) {
  printer->Print(variables_,
    "$name$_ = other.$name$_;\n");
}

void PrimitiveFieldGenerator::GenerateCodecCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "pb::FieldCodec.For$capitalized_type_name$($tag$, $default_value$)");
}

void PrimitiveFieldGenerator::GenerateIsInitialized(io::Printer* printer) {
  if (descriptor_->is_required()) {
    printer->Print(
      variables_,
      "if (!$has_property_check$) {\n"
      "  return false;\n"
      "}\n");
  }
}

void PrimitiveFieldGenerator::GenerateExtensionCode(io::Printer* printer) {
  WritePropertyDocComment(printer, descriptor_);
  printer->Print(
    variables_,
    "$access_level$ static readonly pb::Extension<$extended_type$, $type_name$> $property_name$ =\n"
    "  new pb::Extension<$extended_type$, $type_name$>(");
  GenerateCodecCode(printer);
  printer->Print(");\n");
}

PrimitiveOneofFieldGenerator::PrimitiveOneofFieldGenerator(
    const FieldDescriptor* descriptor, const Options *options)
    : PrimitiveFieldGenerator(descriptor, options) {
  SetCommonOneofFieldVariables(&variables_);
}

PrimitiveOneofFieldGenerator::~PrimitiveOneofFieldGenerator() {
}

void PrimitiveOneofFieldGenerator::GenerateMembers(io::Printer* printer) {
  WritePropertyDocComment(printer, descriptor_);
  AddPublicMemberAttributes(printer);
  printer->Print(
    variables_,
    "$access_level$ $type_name$ $property_name$ {\n"
    "  get { return $has_property_check$ ? ($type_name$) $oneof_name$_ : $default_value$; }\n"
    "  set {\n");
  if (is_value_type) {
    printer->Print(
      variables_,
      "    $oneof_name$_ = value;\n");
  } else {
    printer->Print(
      variables_,
      "    $oneof_name$_ = pb::ProtoPreconditions.CheckNotNull(value, \"value\");\n");
  }
  printer->Print(
    variables_,
    "    $oneof_name$Case_ = $oneof_property_name$OneofCase.$property_name$;\n"
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

void PrimitiveOneofFieldGenerator::GenerateMergingCode(io::Printer* printer) {
  printer->Print(variables_, "$property_name$ = other.$property_name$;\n");
}

void PrimitiveOneofFieldGenerator::WriteToString(io::Printer* printer) {
  printer->Print(variables_,
    "PrintField(\"$descriptor_name$\", $has_property_check$, $oneof_name$_, writer);\n");
}

void PrimitiveOneofFieldGenerator::GenerateParsingCode(io::Printer* printer) {
    printer->Print(
      variables_,
      "$property_name$ = input.Read$capitalized_type_name$();\n");
}

void PrimitiveOneofFieldGenerator::GenerateCloningCode(io::Printer* printer) {
  printer->Print(variables_,
    "$property_name$ = other.$property_name$;\n");
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
