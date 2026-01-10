---
trigger: always_on
---

# Documentation Check Rule
Before planning any code changes:
1. **Review Architecture & Design**: Check [docs/design.md](../../../docs/design.md) for system architecture, component interactions, and technical specifications.
2. **Verify Requirements**: Consult [docs/prd.md](../../../docs/prd.md) to ensure changes align with product requirements, API specifications, and feature definitions.
3. **Understand User Impact**: Read [docs/prfaq.md](../../../docs/prfaq.md) to consider how changes affect users, compatibility, and use cases.
4. **Check Version Compatibility**: Verify that changes maintain backward compatibility as specified in the documentation.
5. **Consider Format Support**: For changes affecting file formats (NetCDF, CDF, GeoTIFF), ensure compliance with format specifications in the documentation.
6. **Review Build Systems**: For build system changes, ensure both CMake and Autotools configurations are updated consistently.
7. **Update Documentation**: Plan to update relevant documentation if implementing new features or changing existing behavior.
