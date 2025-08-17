/**
 * PM2040 – UF2 Multi-ROM Patcher
 *
 * Core logic to inject multiple Pokémon Mini games into UF2 firmware.
 *
 * Original project:    PM2040 – An RP2040-based Flash cart for the Pokémon mini handheld
 * Author:              zwenergy
 * Repository:          https://github.com/zwenergy/PM2040
 */


function lendian32( arr, base ) {
  return ( arr[ base ] + ( arr[ base + 1 ] * 256 ) + ( arr[ base + 2 ] * 65536 ) + ( arr[ base + 3 ] * 16777216 ) );
}

function saveByteArray( filename, data) {
  var blob = new Blob([data], {type: "application/octet-stream"});
  var link = document.createElement('a');
  link.href = window.URL.createObjectURL(blob);
  var fileName = filename;
  link.download = filename;
  link.click();
};

function patchArea( uf2bytearray, uf2Offset, patchArray, len ) {
  const uf2chunk = 512;
  const dataoffset = 32;
  const addroffset = 12;
  const datasizeoffset = 16;

  var uf2array = new Uint8Array( uf2bytearray );

  // Start patching. Go over each chunk.
  for ( let i = 0; i < uf2array.length; i += uf2chunk ) {
    // Get data size
    var datSize = lendian32( uf2array, i + datasizeoffset );
    var baseAddr = lendian32( uf2array, i + addroffset );

    // Contains ROM data?
    if ( ( uf2Offset >= baseAddr && uf2Offset < baseAddr + datSize ) || // ROM start inside chunk.
         ( uf2Offset + len > uf2Offset && uf2Offset + len < baseAddr + datSize ) || // ROM end inside chunk.
         ( uf2Offset < baseAddr && uf2Offset + len > baseAddr + datSize ) ) { // ROM spans over chunk.

      // This chunk contains ROM data.
      // Go over data of chunk.
      for ( let datInd = dataoffset; datInd < dataoffset + datSize; datInd++ ) {
        // Calc addr.
        var curAddr = datInd - dataoffset + baseAddr;
        // Patching byte?
        if ( curAddr >= uf2Offset && curAddr < uf2Offset + len ) {
          var romByteAddr = curAddr - uf2Offset;
          // Patch it.
          uf2array[ i + datInd ] = patchArray[ romByteAddr ];
        }
      }

    }

  }

}

function injectROMs( uf2bytearray, ROMStorage ) {
  var uf2array = new Uint8Array( uf2bytearray );

  // First, we care about the ROMs.
  // Start with the ROM Offset.
  const uf2chunk = 512;
  const dataoffset = 32;
  const addroffset = 12;
  const datasizeoffset = 16;

  const maxLabelSize = 20;
  const labelPlaceholder = "SLOT ";

  console.log( "inject roms" );

  const ROMSize = 524288;

  var foundStart = 0;
  var ROMaddr = 0;

  for ( let i = 0; i < uf2array.length; i += uf2chunk ) {
    // Get data size
    var datSize = lendian32( uf2array, i + datasizeoffset );

    // Go over the data.
    for ( let datInd = dataoffset; datInd < dataoffset + datSize; datInd++ ) {
      if ( String.fromCharCode( uf2array[ i + datInd ] ) == 'R' &&
        String.fromCharCode( uf2array[ i + datInd + 1 ] ) == 'O' &&
        String.fromCharCode( uf2array[ i + datInd + 2 ] ) == 'M' &&
        String.fromCharCode( uf2array[ i + datInd + 3 ] ) == 'S' &&
        String.fromCharCode( uf2array[ i + datInd + 4 ] ) == 'T' &&
        String.fromCharCode( uf2array[ i + datInd + 5 ] ) == 'A' &&
        String.fromCharCode( uf2array[ i + datInd + 6 ] ) == 'R' &&
        String.fromCharCode( uf2array[ i + datInd + 7 ] ) == 'T' ) {

        // Get ROM base addr.
        var chunkBase = lendian32( uf2array, i + addroffset );
        ROMaddr = chunkBase + datInd - dataoffset;

        console.log( "FOUND ROMSTART" );
        console.log( ROMaddr );
        foundStart = 1;
        break;
      }
    }
  }

  if ( foundStart == 0 ) {
    console.log( "did not find" );
    return;
  }

  // Now go over all ROMs.
  for ( let i = 0; i < ENTRIES; ++i ) {
    if ( ROMStorage[ i ] ) {
      let curROMOffset = ROMSize * i + ROMaddr;

      romarray = new Uint8Array( ROMStorage[ i ] )

      // Start patching. Go over each chunk.
      romLen = romarray.length;
      patchArea( uf2bytearray, curROMOffset, romarray, romLen );
    }
  }

  // And now the labels. Search for the base label.
  let labelBaseAddr = 0;
  for ( let i = 0; i < uf2array.length; i += uf2chunk ) {
    // Get data size
    var datSize = lendian32( uf2array, i + datasizeoffset );

    // Go over the data.
    for ( let datInd = dataoffset; datInd < dataoffset + datSize; datInd++ ) {
      if ( String.fromCharCode( uf2array[ i + datInd ] ) == 'S' &&
        String.fromCharCode( uf2array[ i + datInd + 1 ] ) == 'L' &&
        String.fromCharCode( uf2array[ i + datInd + 2 ] ) == 'O' &&
        String.fromCharCode( uf2array[ i + datInd + 3 ] ) == 'T' &&
        String.fromCharCode( uf2array[ i + datInd + 4 ] ) == ' ' &&
        String.fromCharCode( uf2array[ i + datInd + 5 ] ) == '1' ) {

        // Get ROM base addr.
        var chunkBase = lendian32( uf2array, i + addroffset );
        labelBaseAddr = chunkBase + datInd - dataoffset;

        console.log( "FOUND GAME LABEL START" );
        console.log( labelBaseAddr );
        foundStart = 1;
        break;
      }
    }
  }

  // Build replacement.
  let labels = [];
  //let emptyLabel = "EMPTY SLOT ";
  let emptyLabel = "";
  for ( let i = 0; i < ENTRIES; ++i ) {
    if ( ROMStorage[ i ] ) {
      // Get the label.
      let curLabel = document.querySelector("#romLabel" + i.toString() ).value;
      console.log( "cur label len: " + curLabel.length.toString() )

      for ( let j = 0; j < maxLabelSize; j++ ) {
        if ( j >= curLabel.length ) {
          labels.push( Math.min( 255, " ".charCodeAt( 0 ) ) );
        } else {
          labels.push( Math.min( 255, curLabel.charCodeAt( j ) ) );
        }
      }

    } else {
      for ( let j = 0; j < maxLabelSize; j++ ) {
        labels.push( Math.min( 255, emptyLabel.charCodeAt( j ) ) );
      }
    }

    // Add zero byte.
    labels.push( 0 );
  }

  // Create byte array.
  labelByteArray = new Uint8Array( labels );
  patchArea( uf2bytearray, labelBaseAddr, labelByteArray, labels.length );

  // Save the patched file.
  saveByteArray( "PM2040_PATCHED.uf2", uf2bytearray );
}