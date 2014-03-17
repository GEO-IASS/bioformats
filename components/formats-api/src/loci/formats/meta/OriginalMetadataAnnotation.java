/*
 * #%L
 * OME Bio-Formats API for reading and writing file formats.
 * %%
 * Copyright (C) 2005 - 2013 Open Microscopy Environment:
 *   - Board of Regents of the University of Wisconsin-Madison
 *   - Glencoe Software, Inc.
 *   - University of Dundee
 * %%
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of any organization.
 * #L%
 */

package loci.formats.meta;

import loci.common.xml.XMLTools;
import ome.xml.model.XMLAnnotation;

import org.w3c.dom.Document;
import org.w3c.dom.Element;

public class OriginalMetadataAnnotation extends XMLAnnotation {

  public static final String ORIGINAL_METADATA_NS;
  static {
    ORIGINAL_METADATA_NS = "openmicroscopy.org/OriginalMetadata";
  }

  private String key, value;

  // -- OriginalMetadataAnnotation methods --

  public void setKeyValue(String key, String value) {
    setNamespace(ORIGINAL_METADATA_NS);
    this.key = key;
    this.value = value; // Not XML value
    Document doc = XMLTools.createDocument();
    Element r = makeOriginalMetadata(doc);
    super.setValue(XMLTools.dumpXML(null,  doc, r, false));
  }

  // -- XMLAnnotation methods --

  public String getKey() {
    return key;
  }

  /**
   * Return just the value (i.e. v in k=v) as opposed to the XML
   * value which contains the entire block (e.g. &lt;originalmetadata&gt;...)
   */
  public String getValueForKey() {
    return value;
  }

  protected Element makeOriginalMetadata(Document document) {

    Element keyElement =
      document.createElement("Key");
    Element valueElement =
      document.createElement("Value");
    keyElement.setTextContent(key);
    valueElement.setTextContent(value);

    Element originalMetadata =
      document.createElement("OriginalMetadata");
    originalMetadata.appendChild(keyElement);
    originalMetadata.appendChild(valueElement);

    return originalMetadata;
  }

}