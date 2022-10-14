<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
    <xsl:output method="html" encoding="UTF-8"/>
    <!-- shaun.roe@cern.ch 21 September 2005 -->
    <!-- default to no output -->
    <xsl:template match="text()|@*"> </xsl:template>
    <xsl:template match="oks-schema">
        <html>
            <head>
                <title> OKS Schema Documentation</title>
                <style type="text/css"> /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
                    /* More-configurable styles */ 
                    /******** General ********/ 
                    /* Document body */
                    body { color: Black; background-color: White; font-family: Arial, sans-serif;
                    font-size: 10pt; } 
                    /* Horizontal rules */
                    hr { color: black; } 
                    /* Document title*/ 
                    h1 { font-size: 18pt; letter-spacing: 2px; border-bottom: 1px #ccc solid;
                    padding-top: 5px; padding-bottom: 5px; } 
                    /* Main section headers */ 
                    h2 {font-size: 14pt; letter-spacing: 1px; } 
                    /* Sub-section headers */ 
                    h3, h3 a, h3 span { font-size: 12pt; font-weight: bold; color: black; } 
                    /* Table displaying the properties of the schema components or the schema document itself */
                    table.properties th, table.properties th a { color: black; background-color:
                    #F99; /* Pink */ } table.properties td { background-color: #eee; /* Gray */}
                    /* Table displaying the relationships of the classes*/
                    table.relationship th, table.relationship th a { color: black; background-color:
                    #369; /* Blue*/ } table.relationship td { background-color: #eee; /* Gray */ }
                    /* Table displaying the methods of the classes*/
                    table.methods th, table.methods th a { color: black; background-color:
                    #F99; /* Pink*/ } table.methods td { background-color: #eee; /* Gray */ }
                    /******** Table of Contents Section ********/ 
                    
                    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */ 
                    /* Base styles */
                    /******** General ********/ 
                    /* Unordered lists */ 
                    ul { margin-left: 1.5em; margin-bottom: 0em; } 
                    /* Tables */ 
                    table { margin-top: 10px; margin-bottom:
                    10px; margin-left: 2px; margin-right: 2px; } table th, table td { font-size:
                    10pt; vertical-align: top; padding-top: 3px; padding-bottom: 3px; padding-left:
                    10px; padding-right: 10px; } table th {  text-align: left; 
                    height: 40px;}
                    /* Table displaying the attributes or relationships */ 
                    table.properties { width: 90%; } table.properties th { font-weight: bold; } 
                    table.relationship { width: 90%; } table.relationship th { font-weight: bold; } 
                    table.methods { width: 90%; } table.methods th { font-weight: normal; }
                     
                   </style>
                <meta http-equiv="Content-Type" content="text/xml; charset=UTF-8"/>
            </head>
            <body>
                <h1>
                    <a name="top">OKS Schema Documentation: <xsl:value-of select="./info/@name"
                    /></a>
                </h1>
                <div/>
                <div style="text-align: right; clear: both;">
                    <a href="#top">top</a>
                </div>
                <h2>Table of contents</h2>
                <ul>
                <xsl:apply-templates mode="buildIndex"/>
                </ul>
                <hr/>
                <xsl:apply-templates mode="insertValues"/>
            </body>
        </html>
    </xsl:template>
    <xsl:template match="info|class" mode="buildIndex">
        <li>
            <a href="#{@name}"><xsl:value-of select="concat(name(),': ',@name)"/></a>
        </li>
    </xsl:template>
    <xsl:template match="info" mode="insertValues">
        <h2>
            <a name="{@name}">Schema Document Properties from "info"</a>
        </h2>
        <table class="properties">
            <tr>
                <th> Name </th>
                <td>OKS info</td>
            </tr>
            <tr>
                <th>
                    <xsl:value-of select="@name"/>
                </th>
                <td>
                    <ul>
                        <li>This schema file contains <xsl:value-of select="@num-of-items"/> items.</li>
                        <li>It was created by <xsl:value-of select="@created-by"/> on <xsl:value-of
                                select="substring-before(@creation-time,' ')"/>.</li>
                        <li>The last modification was made on <xsl:value-of
                                select="substring-before(@last-modification-time,' ')"/></li>
                    </ul>
                </td>
            </tr>
        </table>
        <hr/>
    </xsl:template>
    <xsl:template match="class" mode="insertValues">
        <div style="text-align: right; clear: both;">
            <a href="#top">top</a>
        </div>
        <xsl:variable name="className" select="@name"/>
        <h3>Class: <a name="{$className}" class="name">
                <xsl:value-of select="$className"/>
        </a></h3>
        <xsl:value-of select="concat($className,' is ')"/>
        <xsl:choose>
            <xsl:when test="@is-abstract = 'yes'">an abstract </xsl:when>
            <xsl:otherwise>a concrete </xsl:otherwise>
        </xsl:choose>
        <xsl:choose>
            <xsl:when test="superclass">
                <xsl:text>subclass of </xsl:text>
                <b>
                    <xsl:variable name="numSuperClass" select="count(superclass)"/>
            <xsl:for-each select="superclass">
                
                <a href="#{@name}"><xsl:value-of select="@name"/></a>
                <xsl:if test="position() != $numSuperClass"><xsl:text>, </xsl:text></xsl:if>
            </xsl:for-each>
                </b>.
            </xsl:when>
            <xsl:otherwise>class.</xsl:otherwise>
        </xsl:choose>
        <p>
        <xsl:value-of select="@description"/></p>
        <xsl:choose>
            <xsl:when test="attribute">
                <h4>Attributes</h4>
            
                <table class="properties">
                    <tr>
                        <th>Name</th>
                        <th>Type</th>
                        <th>Format</th>
                        <th>Range</th>
                        <th>Initial value</th>
                        <th>Multi-valued?</th>
                        <th>Is-not-null?</th>
                        <th>Description</th>
                    </tr>
                    <xsl:for-each select="attribute">
                    <tr>
                        <td><b><xsl:value-of select="@name"/></b></td>
                        <td>
                            <xsl:value-of select="@type"/>
                        </td>
                        <td>
                            <xsl:choose>
                                <xsl:when test="contains('s8s16s32u8u16u32',@type)">
                                    <xsl:value-of select="@format"/>
                                </xsl:when>
                                <xsl:otherwise></xsl:otherwise>
                            </xsl:choose>

                        </td>
                        <td>
                            <xsl:value-of select="translate(@range,',',' ')"/>
                        </td>
                        <td>
                            <xsl:choose>
                                <xsl:when test="@init-value">
                                    <xsl:value-of select="@init-value"/>
                                </xsl:when>
                                <xsl:otherwise>
                                    <i>None given</i>
                                </xsl:otherwise>
                            </xsl:choose>
                        </td>
                        
                        <xsl:variable name="listOrVector">
                            <xsl:choose>
                                <xsl:when test="@multi-value-implementation = 'vector'">
                                    <xsl:value-of select="'vector'"/>
                                </xsl:when>
                                <xsl:otherwise>
                                    <xsl:value-of select="'list'"/>
                                </xsl:otherwise>
                            </xsl:choose>
                        </xsl:variable>
                        
                        <td>
                            <xsl:choose>
                                <xsl:when test="@is-multi-value = 'yes'"> yes, implemented as a
                                    <xsl:value-of select="$listOrVector"/></xsl:when>
                                <xsl:otherwise>no</xsl:otherwise>
                            </xsl:choose>
                        </td>
                    
                        
                        <td>
                            <xsl:choose>
                                <xsl:when test="@is-not-null = 'yes'"> yes
                                    </xsl:when>
                                <xsl:otherwise>no</xsl:otherwise>
                            </xsl:choose>
                        </td>
                        <td>
                            <xsl:choose>
                                <xsl:when test="@description">
                                    <xsl:value-of select="@description"/>
                                </xsl:when>
                                <xsl:otherwise>
                                    <i>None given</i>
                                </xsl:otherwise>
                            </xsl:choose>
                        </td>
                    </tr>
                    </xsl:for-each>
                </table>
            
            </xsl:when>
            <xsl:otherwise>This class has no attributes.</xsl:otherwise>
        </xsl:choose>      
        <xsl:choose>
            <xsl:when test="relationship">
                <h4>Relationships</h4>
                
                    <table class="relationship">
                        
                        <tr>
                            <th>Name</th>
                            
                            <th>Related to class</th>
                            <th>Cardinality</th>
                            <th>Type of relationship</th>
                            <th>Description</th>
                        </tr>
                        <xsl:for-each select="relationship">
                            <xsl:variable name="composite">
                                <xsl:choose>
                                    <xsl:when test="@is-composite = 'yes'"><xsl:value-of
                                        select="'composite'"/></xsl:when>
                                    <xsl:otherwise><xsl:value-of
                                        select="'non-composite'"/></xsl:otherwise>
                                </xsl:choose>
                            </xsl:variable>
                            <xsl:variable name="exclusive">
                                <xsl:choose>
                                    <xsl:when test="@is-exclusive = 'yes'"><xsl:value-of
                                        select="'exclusive'"/></xsl:when>
                                    <xsl:otherwise><xsl:value-of
                                        select="'shared'"/></xsl:otherwise>
                                </xsl:choose>
                            </xsl:variable>
                            <xsl:variable name="dependent">
                                <xsl:choose>
                                    <xsl:when test="@is-dependent = 'yes'"><xsl:value-of
                                        select="'dependent'"/></xsl:when>
                                    <xsl:otherwise><xsl:value-of
                                        select="'independent'"/></xsl:otherwise>
                                </xsl:choose>
                            </xsl:variable>
                            <tr>
                            <td>
                                <b>
                                    <xsl:value-of select="@name"/>
                                </b>
                            </td>
                                
                                <td><a href="#{@class-type}"><xsl:value-of select="@class-type"/></a></td>
                                <td>From <xsl:value-of select="@low-cc"/> to <xsl:value-of select="@high-cc"/></td>
                                <td><xsl:value-of select="concat($composite,', ',$exclusive,
                                    ', ',$dependent)"/></td>
                                <td>
                                    <xsl:choose>
                                        <xsl:when test="@description">
                                            <xsl:value-of select="@description"/>
                                        </xsl:when>
                                        <xsl:otherwise>
                                            <i>None given</i>
                                        </xsl:otherwise>
                                    </xsl:choose>
                                </td>
                            </tr>
                        </xsl:for-each>
                    </table>
                
            </xsl:when>
            <xsl:otherwise>This class has no relationships.</xsl:otherwise>
        </xsl:choose>
        <xsl:if test="method">
            <h4>Methods</h4>
            <table class="methods">
                <xsl:for-each select="method">
                    <tr><th colspan="2"><b><xsl:value-of select="@name"/></b><br/>
                    <p><xsl:value-of select="@description"/></p></th></tr>
                    
                    <xsl:for-each select="method-implementation">
                        <tr><th><b><xsl:value-of select="@language"/></b></th>
                        <td><xsl:value-of select="@prototype"/>
                        <p><xsl:value-of select="@body"/></p>
                        </td></tr>
                    </xsl:for-each>
                </xsl:for-each>
            </table>
        </xsl:if>
        <hr/>
    </xsl:template>
</xsl:stylesheet>
