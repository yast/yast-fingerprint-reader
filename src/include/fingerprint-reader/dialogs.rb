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

# File:	include/fingerprint-reader/dialogs.ycp
# Package:	Configuration of fingerprint-reader
# Summary:	Dialogs definitions
# Authors:	Jiri Suchomel <jsuchome@suse.cz>
#
# $Id$
module Yast
  module FingerprintReaderDialogsInclude
    def initialize_fingerprint_reader_dialogs(include_target)
      Yast.import "UI"

      textdomain "fingerprint-reader"

      Yast.import "Confirm"
      Yast.import "FileUtils"
      Yast.import "FingerprintReader"
      Yast.import "Label"
      Yast.import "Package"
      Yast.import "Popup"
      Yast.import "Report"
      Yast.import "Stage"
      Yast.import "Wizard"

      Yast.include include_target, "fingerprint-reader/helps.rb"
    end

    def ReallyAbort
      !FingerprintReader.modified || Popup.ReallyAbort(true)
    end

    # Read settings dialog
    # @return `abort if aborted and `next otherwise
    def ReadDialog
      Wizard.RestoreHelp(Ops.get_string(@HELPS, "read", ""))
      return :abort if !Confirm.MustBeRoot
      ret = FingerprintReader.Read
      ret ? :next : :abort
    end

    # Write settings dialog
    # @return `abort if aborted and `next otherwise
    def WriteDialog
      Wizard.RestoreHelp(Ops.get_string(@HELPS, "write", ""))
      ret = FingerprintReader.Write
      ret ? :next : :abort
    end

    # Main configuration dialog
    # @return dialog result
    def FingerprintReaderDialog
      # FingerprintReader summary dialog caption
      caption = _("Fingerprint Reader Configuration")

      use_pam = FingerprintReader.use_pam
      # help text
      help_text = _(
        "<p>\n" +
          "<b>Fingerprint Authentication</b><br>\n" +
          "The fingerprint reader configuration updates your PAM settings to enable authentication with fingerprints.</p>\n"
      ) +
        # help text, cont.
        _(
          "<p>To register fingerprints for a user, start YaST User Management and see the Plug-Ins tab of the selected user. To automatically start YaST User Management, activate <b>Start User Management After Finish</b>.</p>"
        )

      con = HBox(
        HSpacing(3),
        VBox(
          # frame label
          Frame(
            _("User Authentication"),
            HBox(
              HSpacing(0.5),
              VBox(
                VSpacing(0.5),
                RadioButtonGroup(
                  Id(:rd),
                  Left(
                    HVSquash(
                      VBox(
                        Left(
                          RadioButton(
                            Id(:pamno),
                            Opt(:notify),
                            # radio button label
                            _("Do No&t Use Fingerprint Reader"),
                            !use_pam
                          )
                        ),
                        Left(
                          RadioButton(
                            Id(:pamyes),
                            Opt(:notify),
                            # radio button label
                            _("&Use Fingerprint Reader"),
                            use_pam
                          )
                        )
                      )
                    )
                  )
                ),
                VSpacing(0.5)
              ),
              HSpacing(0.5)
            )
          ),
          VSpacing(),
          Left(
            # button label
            CheckBox(Id(:users), _("&Start User Management after Finish"))
          )
        ),
        HSpacing(3)
      )

      Wizard.SetContentsButtons(
        caption,
        con,
        help_text,
        Stage.cont ? Label.BackButton : Label.CancelButton,
        Stage.cont ? Label.NextButton : Label.FinishButton
      )
      Wizard.SetDesktopTitleAndIcon("fingerprint-reader")
      Wizard.HideAbortButton if !Stage.cont

      UI.ChangeWidget(Id(:users), :Enabled, use_pam)

      ret = nil
      while true
        ret = UI.UserInput

        if ret == :pamyes || ret == :pamno
          use_pam = ret == :pamyes
          UI.ChangeWidget(Id(:users), :Enabled, use_pam)
        end

        if ret == :abort || ret == :cancel || ret == :back
          if ReallyAbort()
            break
          else
            next
          end
        elsif ret == :users

        elsif ret == :next
          if use_pam && !Package.InstallAll(FingerprintReader.required_packages)
            use_pam = false
            UI.ChangeWidget(Id(:rd), :Value, :pamno)
            UI.ChangeWidget(Id(:users), :Enabled, false)
            next
          end
          if use_pam != FingerprintReader.use_pam
            FingerprintReader.modified = true
            FingerprintReader.use_pam = use_pam
          end
          if use_pam && UI.QueryWidget(Id(:users), :Value) == true
            FingerprintReader.run_users = true
          end
          break
        end
      end
      deep_copy(ret)
    end
  end
end
