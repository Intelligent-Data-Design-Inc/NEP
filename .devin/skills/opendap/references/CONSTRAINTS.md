# OPeNDAP Constraint Expressions

This document provides comprehensive examples and patterns for OPeNDAP constraint expressions.

## Constraint Expression Basics

A constraint expression has two parts:

```
URL?projection&selection
```

- **Projection**: What data to return (variables, array subsets)
- **Selection**: How to filter the data (boolean conditions)

Both parts are optional. Either or both can be used.

## Projection Expressions

### Selecting Variables

**Single variable**:
```
?temperature
```

**Multiple variables**:
```
?temperature,salinity,pressure
```

**Structure fields**:
```
?station.latitude,station.longitude,station.time
```

**Nested structures**:
```
?cruise.station.cast.temperature
```

### Array Subsetting

**Single element**:
```
?sst[0][10][20]
```

**Range (start:stop)**:
```
?sst[0:10][20:30][40:50]
```

**Stride (start:stride:stop)**:
```
?sst[0:2:100][0:5:50][0:10:180]
# Every 2nd time, every 5th lat, every 10th lon
```

**Open-ended ranges**:
```
?sst[10:]      # From index 10 to end
?sst[:100]     # From start to index 100
?sst[:]        # Entire dimension
```

### Grid Subsetting

When subsetting a Grid, include coordinate variables:

**Without coordinates** (just the array):
```
?sst[0:10][20:30][40:50]
```

**With coordinates**:
```
?time[0:10],lat[20:30],lon[40:50],sst[0:10][20:30][40:50]
```

**Using geogrid() function** (Hyrax servers):
```
?geogrid(sst, north_lat, west_lon, south_lat, east_lon)
?geogrid(sst, 62, 206, 56, 210)
```

## Selection Expressions

### Comparison Operators

**Numeric comparisons**:
```
?station&station.temperature>20.0
?station&station.depth<100
?station&station.salinity>=34.5
?station&station.pressure<=1000
?station&station.id=12345
?station&station.quality!=0
```

**String comparisons**:
```
?station&station.name="Station_A"
?station&station.type!="reference"
```

**String pattern matching** (regex):
```
?station&station.comment~=".*shark.*"
?station&station.location~="^North.*"
```

### Multiple Conditions

**AND conditions** (multiple & clauses):
```
?station&station.lat>0.0&station.lon<-60.0
?station&station.temp>20&station.depth<50&station.salinity>34
```

**OR conditions** (using lists):
```
?station&station.month={4,5,6,7}
?station&station.type={"CTD","XBT","profiler"}
```

**Combining variables in lists**:
```
?station&station.month={4,5,6,station.monsoon_month}
```

### Range Conditions

**Value between bounds**:
```
?station&station.temp>15&station.temp<25
?data&10<data.value<100
```

**Time ranges**:
```
?data&data.time>19722&data.time<19755
```

## Sequence Operations

Sequences are like database tables with rows of data.

### Selecting Fields

**Specific fields**:
```
?sequence.field1,sequence.field2,sequence.field3
```

**All fields with filter**:
```
?sequence&sequence.temperature>20
```

### Filtering Rows

**Single condition**:
```
?URI_GSO-Dock.Time,URI_GSO-Dock.Sea_Temp&URI_GSO-Dock.Time<35234.1
```

**Multiple conditions**:
```
?station.cast.press,station.cast.temp&station.cast.press>500.0
?station.cast&station.cast.temp>22.0
```

**Complex filters**:
```
?station&station.lat>0.0&station.month={4,5,6,7}
```

## Server Functions

### geogrid() - Geographic Subsetting

**Syntax**:
```
geogrid(variable, top, left, bottom, right, [other_expressions])
```

**Example**:
```
?geogrid(sst, 62, 206, 56, 210, "19722<time<19755")
```

**Multiple constraints**:
```
?geogrid(sst, 62, 206, 56, 210, "19722<time", "time<19755")
```

### linear_scale() - Scale Data Values

**Syntax**:
```
linear_scale(variable, scale_factor, offset)
linear_scale(variable)  # Uses metadata
```

**Example**:
```
?linear_scale(sst, 0.01, 0)
```

**Pipelined with geogrid**:
```
?linear_scale(geogrid(sst, 62, 206, 56, 210), 0.01, 0)
```

### version() - List Available Functions

**Query**:
```
?version()
```

**Get function help**:
```
?geogrid()
?linear_scale()
```

## Advanced Patterns

### Combining Projections and Selections

**Array subset with filter**:
```
?sst[0:100][0:50]&time>19722
```

**Multiple variables with conditions**:
```
?lat,lon,temp,salinity&temp>20&salinity>34
```

### Working with Nested Structures

**Nested sequence**:
```
?cruise.station.cast.depth,cruise.station.cast.temp
```

**Filter on nested field**:
```
?cruise.station&cruise.station.latitude>0
```

**Multiple levels**:
```
?cruise.station.cast&cruise.station.cast.temp>20
```

### Sampling Patterns

**Every Nth element**:
```
?sst[0:10:1857][0:5:89][0:10:180]
```

**Sparse sampling**:
```
?sst[::100][::10][::20]  # Every 100th time, 10th lat, 20th lon
```

**Diagonal sampling** (if supported):
```
?array[0:10][0:10]  # 11x11 subset
```

## Common Use Cases

### 1. Time Series at a Point

```
?time,sst[0:1857][44][90]
# All times, single lat/lon point
```

### 2. Spatial Subset at One Time

```
?lat[20:40],lon[100:140],sst[0][20:40][100:140]
# Single time, regional subset
```

### 3. Vertical Profile

```
?depth,temperature[0:500],salinity[0:500]
# Full depth profile
```

### 4. Quality-Filtered Data

```
?time,temp,salinity&quality_flag=1
# Only high-quality data
```

### 5. Regional and Temporal Subset

```
?geogrid(sst, 45, -130, 30, -110, "19900<time<20000")
# Pacific Northwest, specific time range
```

### 6. Multi-Variable Analysis

```
?time,lat,lon,sst,wind_speed&sst>25&wind_speed<5
# Warm, calm conditions
```

## Pattern Matching

### Regular Expression Syntax

**Wildcards**:
- `.` - Any single character
- `.*` - Zero or more characters
- `.+` - One or more characters
- `.?` - Zero or one character

**Anchors**:
- `^` - Start of string
- `$` - End of string

**Character classes**:
- `[abc]` - Match a, b, or c
- `[0-9]` - Match any digit
- `[^0-9]` - Match any non-digit

**Examples**:
```
?station&station.comment~=".*shark.*"      # Contains "shark"
?station&station.name~="^Station_[0-9]+$"  # Station_123 format
?data&data.type~="CTD|XBT|profiler"        # Multiple types
```

## Error Handling

### Common Constraint Expression Errors

**Invalid variable name**:
```
Error: Variable 'sst_temp' not found
Fix: Check DDS/DMR for correct name
```

**Index out of bounds**:
```
Error: Array index [2000] exceeds dimension size [1857]
Fix: Verify dimension sizes in DDS/DMR
```

**Syntax error**:
```
Error: Expected ']' but found ','
Fix: Check bracket matching and syntax
```

**Type mismatch**:
```
Error: Cannot compare String with Float64
Fix: Use appropriate operators for data type
```

### Testing Constraint Expressions

**Use .ascii for debugging**:
```
http://server/data.nc.ascii?sst[0:1][0:5][0:5]
```

**Check metadata first**:
```
http://server/data.nc.dmr.xml
http://server/data.nc.dds
```

**Test incrementally**:
1. Start with simple projection: `?variable`
2. Add subsetting: `?variable[0:10]`
3. Add selection: `?variable[0:10]&variable>100`

## Performance Tips

### Minimize Data Transfer

**Request only needed variables**:
```
?temp,salinity  # Not ?*
```

**Use appropriate stride**:
```
?sst[0:10:1857]  # Every 10th instead of all
```

**Subset at server**:
```
?sst[0:100][20:40][50:80]  # Not full array
```

### Leverage Server Functions

**Process at server**:
```
?linear_scale(geogrid(sst, 45, -130, 30, -110))
```

**Combine operations**:
```
?mean(sst[0:100][20:40][50:80])  # If server supports
```

### Cache Metadata

**Reuse DDS/DAS/DMR**:
- Cache structure information
- Avoid repeated metadata requests
- Use cached info for constraint construction

## Examples by Data Type

### Gridded Data (Arrays/Grids)

```
# Single point
?sst[100][44][90]

# Regional subset
?sst[0:100][20:40][80:120]

# Time series at point
?time,sst[0:1857][44][90]

# Spatial map at time
?lat,lon,sst[100][0:89][0:180]
```

### Station Data (Sequences)

```
# All stations
?station

# Specific fields
?station.id,station.lat,station.lon,station.time

# Filtered stations
?station&station.lat>0&station.lon<-60

# Quality filtered
?station.temp,station.salinity&station.quality=1
```

### Profile Data (Nested Sequences)

```
# All profiles
?cruise.station.cast

# Specific depths
?cruise.station.cast.depth,cruise.station.cast.temp&cruise.station.cast.depth<100

# Filtered by location
?cruise.station.cast&cruise.station.latitude>30
```

## Best Practices

1. **Always check metadata first** - Use .dds/.dmr.xml to understand structure
2. **Test with .ascii** - Verify constraint expressions before using in code
3. **Use server functions** - Leverage geogrid(), linear_scale(), etc.
4. **Minimize data transfer** - Request only what you need
5. **Handle errors gracefully** - Check HTTP status codes and error messages
6. **Cache when possible** - Reuse metadata and frequently accessed subsets
7. **Document constraints** - Complex expressions can be hard to understand later
