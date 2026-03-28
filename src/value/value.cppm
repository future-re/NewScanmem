module;

export module value;

export import value.types;
export import value.flags;
export import value.parser;
export import value.view;

// Backward compatibility: export old names in value namespace
export namespace value {

// Re-export for compatibility during migration
using ValueVariant = Value;
using ByteArrayData = ByteArray;

}  // namespace value
