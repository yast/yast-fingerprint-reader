# encoding: utf-8

# ------------------------------------------------------------------------------
# Copyright (c) 2006-2012 Novell, Inc. All Rights Reserved.
#
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of version 2 of the GNU General Public License as published by the
# Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, contact Novell, Inc.
#
# To contact Novell about this file by physical or electronic mail, you may find
# current contact information at www.novell.com.
# ------------------------------------------------------------------------------

# File:	clients/fingerprint-reader.ycp
# Package:	Configuration of fingerprint-reader
# Summary:	Main file
# Authors:	Jiri Suchomel <jsuchome@suse.cz>
#
# $Id$
#
# Main file for fingerprint-reader configuration. Uses all other files.
module Yast
  class FingerprintReaderClient < Client
    def main
      Yast.import "UI"

      #**
      # <h3>Configuration of fingerprint-reader</h3>

      textdomain "fingerprint-reader"

      # The main ()
      Builtins.y2milestone("----------------------------------------")
      Builtins.y2milestone("FingerprintReader module started")

      Yast.import "CommandLine"
      Yast.import "FingerprintReader"

      Yast.include self, "fingerprint-reader/wizards.rb"

      @cmdline_description = {
        "id"         => "fingerprint-reader",
        # Command line help text for the Xfingerprint-reader module
        "help"       => _(
          "Configuration of fingerprint-reader"
        ),
        "guihandler" => fun_ref(method(:FingerprintReaderSequence), "any ()"),
        "initialize" => fun_ref(FingerprintReader.method(:Read), "boolean ()"),
        "finish"     => fun_ref(FingerprintReader.method(:Write), "boolean ()"),
        "actions"    => {
          "enable"  => {
            "handler" => fun_ref(method(:EnableHandler), "boolean (map)"),
            # command line help text for 'enable' action
            "help"    => _(
              "Enable fingerprint authentication"
            )
          },
          "disable" => {
            "handler" => fun_ref(method(:DisableHandler), "boolean (map)"),
            # command line help text for 'disable' action
            "help"    => _(
              "Disable fingerprint authentication"
            )
          },
          "summary" => {
            "handler" => fun_ref(method(:SummaryHandler), "boolean (map)"),
            # command line help text for 'summary' action
            "help"    => _(
              "Configuration summary"
            )
          }
        },
        "options"    => {},
        "mappings"   => { "enable" => [], "disable" => [], "summary" => [] }
      }

      @ret = CommandLine.Run(@cmdline_description)

      Builtins.y2milestone("FingerprintReader module finished with %1", @ret)
      Builtins.y2milestone("----------------------------------------")

      if FingerprintReader.run_users
        Builtins.y2milestone("users should be called after FingerprintReader")
        @ret = WFM.CallFunction("users", WFM.Args)
        Builtins.y2milestone("Users module returned %1", @ret)
      end

      deep_copy(@ret) 

      # EOF
    end

    # Enable the Fingerprint reader
    # @param [Hash] options  a list of parameters passed as args
    # @return [Boolean] true on success
    def EnableHandler(options)
      options = deep_copy(options)
      if !FingerprintReader.use_pam
        FingerprintReader.use_pam = false
        FingerprintReader.modified = true
      end
      FingerprintReader.modified
    end

    # Disable the Fingerprint reader
    # @param [Hash] options  a list of parameters passed as args
    # @return [Boolean] true on success
    def DisableHandler(options)
      options = deep_copy(options)
      if FingerprintReader.use_pam
        FingerprintReader.use_pam = true
        FingerprintReader.modified = true
      end
      FingerprintReader.modified
    end

    # Print summary of basic options
    # @return [Boolean] false
    def SummaryHandler(options)
      options = deep_copy(options)
      CommandLine.Print(
        FingerprintReader.use_pam ?
          # summary item
          _("Use Fingerprint Authentication") :
          # summary item
          _("Do Not Use Fingerprint Authentication")
      )
      false # do not call Write...
    end
  end
end

Yast::FingerprintReaderClient.new.main
