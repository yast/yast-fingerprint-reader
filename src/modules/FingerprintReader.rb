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

# File:	modules/FingerprintReader.ycp
# Package:	Configuration of fingerprint-reader
# Summary:	FingerprintReader settings, input and output functions
# Authors:	Jiri Suchomel <jsuchome@suse.cz>
#
# $Id$
#
# Representation of the configuration of fingerprint-reader.
# Input and output routines.
require "yast"

module Yast
  class FingerprintReaderClass < Module
    def main
      textdomain "fingerprint-reader"

      Yast.import "FileUtils"
      Yast.import "Pam"
      Yast.import "Popup"
      Yast.import "Progress"
      Yast.import "Report"
      Yast.import "Summary"

      # Data was modified?
      @modified = false

      # if yast2-users should be called after fingerprint is set up

      @run_users = false
      # Required packages for this module to operate
      @required_packages = ["pam_fprint"]

      # Write only, used during autoinstallation.
      # Don't run services and SuSEconfig, it's all done at one place.
      @write_only = false

      # if Fingerprint Reader authentication is enabled
      @use_pam = false

      # Directory with fingerprint files that should be imported
      @import_dir = ""
    end

    # Get the list of fingerprint readers
    def ReadFingerprintReaderDevices
      devices = Convert.to_list(SCR.Read(path(".probe.fingerprint")))
      devices = [] if devices == nil
      deep_copy(devices)
    end

    # If pam_mount is enabled, pam_fprint cannot be used (bnc#390810)
    def CryptedHomesEnabled
      enabled = false
      Builtins.foreach(["gdm", "login", "kdm", "xdm", "sudo"]) do |service|
        out = Convert.to_map(
          SCR.Execute(
            path(".target.bash_output"),
            Builtins.sformat("pam-config --service %1 -q --mount", service)
          )
        )
        if Ops.get_integer(out, "exit", 1) == 0
          ll = Builtins.filter(
            Builtins.splitstring(Ops.get_string(out, "stdout", ""), "\n")
          ) { |l| l != "" }
          if Ops.greater_than(Builtins.size(ll), 0)
            enabled = true
            raise Break
          end
        end
      end
      enabled
    end

    # Read all fingerprint-reader settings
    # @return true on success
    def Read
      devices = ReadFingerprintReaderDevices()

      if devices == []
        # error popup: no config of non-existent device
        Report.Error(
          _("Fingerprint reader device is not available on this system.")
        )
        return false
      end
      if CryptedHomesEnabled()
        # error popup (pam_fp cannot work with pam_mount)
        Report.Error(
          _(
            "Fingerprint reader devices cannot be used when encrypted directories are used."
          )
        )
        return false
      end
      @use_pam = Pam.Enabled("fprint")

      @modified = false
      true
    end

    # Write all fingerprint-reader settings
    # @return true on success
    def Write
      return true if !@modified

      # FingerprintReader read dialog captio
      caption = _("Saving Fingerprint Reader Configuration")

      sl = 100
      Builtins.sleep(sl)

      # We do not set help text here, because it was set outside
      Progress.New(
        caption,
        " ",
        2,
        [
          # Progress stage
          _("Write the PAM settings"),
          # Progress stage
          _("Import fingerprint files")
        ],
        [
          # Progress step
          _("Writing the PAM settings..."),
          # Progress step
          _("Importing fingerprint files..."),
          # Progress finished
          _("Finished")
        ],
        ""
      )

      Progress.NextStage

      pam_ret = @use_pam ? Pam.Add("fprint") : Pam.Remove("fprint")
      if !pam_ret
        # Error message
        Report.Error(_("Cannot write PAM settings."))
      end

      Builtins.sleep(sl)

      Progress.NextStage

      Progress.NextStage
      Builtins.sleep(sl)

      true
    end

    # Get all fingerprint-reader settings from the first parameter
    # (For use by autoinstallation.)
    # @param [Hash] settings The YCP structure to be imported.
    # @return [Boolean] True on success
    def Import(settings)
      settings = deep_copy(settings)
      @use_pam = Ops.get_boolean(settings, "use_pam", @use_pam)
      @import_dir = Ops.get_string(settings, "import_dir", @import_dir)
      true
    end

    # Dump the fingerprint-reader settings to a single map
    # (For use by autoinstallation.)
    # @return [Hash] Dumped settings (later acceptable by Import ())
    def Export
      { "use_pam" => @use_pam, "import_dir" => @import_dir }
    end

    # Create a textual summary and a list of unconfigured cards
    # @return summary of the current configuration
    def Summary
      # summary header
      summary = Summary.AddHeader("", _("PAM Login"))

      summary = Summary.AddLine(
        summary,
        @use_pam ?
          # summary item
          _("Use Fingerprint Authentication") :
          # summary item
          _("Do Not Use Fingerprint Authentication")
      )
      [summary, []]
    end

    # Create a short textual summary
    # @return summary of the current configuration
    def ShortSummary
      Builtins.sformat(
        # summary text (yes/no follows)
        _("<b>Fingerprint Authentication Enabled</b>: %1<br>"),
        @use_pam ?
          # summary value
          _("Yes") :
          # summary value
          _("No")
      )
    end

    # Return packages needed to be installed and removed during
    # Autoinstallation to insure module has all needed software
    # installed.
    # @return [Hash] with 2 lists.
    def AutoPackages
      { "install" => @required_packages, "remove" => [] }
    end

    publish :variable => :modified, :type => "boolean"
    publish :variable => :run_users, :type => "boolean"
    publish :variable => :required_packages, :type => "list <string>"
    publish :variable => :write_only, :type => "boolean"
    publish :variable => :use_pam, :type => "boolean"
    publish :variable => :import_dir, :type => "string"
    publish :function => :ReadFingerprintReaderDevices, :type => "list ()"
    publish :function => :CryptedHomesEnabled, :type => "boolean ()"
    publish :function => :Read, :type => "boolean ()"
    publish :function => :Write, :type => "boolean ()"
    publish :function => :Import, :type => "boolean (map)"
    publish :function => :Export, :type => "map ()"
    publish :function => :Summary, :type => "list ()"
    publish :function => :ShortSummary, :type => "string ()"
    publish :function => :AutoPackages, :type => "map ()"
  end

  FingerprintReader = FingerprintReaderClass.new
  FingerprintReader.main
end
