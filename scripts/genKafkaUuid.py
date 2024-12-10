import uuid
import base64

# Generate UUID
generated_uuid = uuid.uuid4()
print(f"Generated UUID: {generated_uuid}")

# Convert UUID to bytes
uuid_bytes = generated_uuid.bytes

# Encode the UUID bytes in base64
base64_uuid = base64.urlsafe_b64encode(uuid_bytes).rstrip(b'=').decode('ascii')
print(f"Base64-encoded UUID: {base64_uuid}")