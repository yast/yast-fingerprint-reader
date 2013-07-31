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

# File:
#	include/users/users_plugin_fingerprint_reader.ycp
#
# Package:
#	Configuration of Fingerprint Reader
#
# Summary:
#	GUI part of plugin (UsersPluginFingerprintReader) used to manage
#	user fingerprints on the appropriate hardware
#
# Authors:
#	Jiri Suchomel <jsuchome@suse.cz>
#
# $Id$
module Yast
  class UsersPluginFingerprintReaderClient < Client
    def main
      Yast.import "UI"
      textdomain "fingerprint-reader"

      Yast.import "Directory"
      Yast.import "Label"
      Yast.import "Users"
      Yast.import "UsersPluginFingerprintReader"
      Yast.import "Wizard"

      @ret = nil
      @func = ""
      @config = {}
      @data = {}

      # Check arguments
      if Ops.greater_than(Builtins.size(WFM.Args), 0) &&
          Ops.is_string?(WFM.Args(0))
        @func = Convert.to_string(WFM.Args(0))
        if Ops.greater_than(Builtins.size(WFM.Args), 1) &&
            Ops.is_map?(WFM.Args(1))
          @config = Convert.convert(
            WFM.Args(1),
            :from => "any",
            :to   => "map <string, any>"
          )
        end
        if Ops.greater_than(Builtins.size(WFM.Args), 2) &&
            Ops.is_map?(WFM.Args(2))
          @data = Convert.convert(
            WFM.Args(2),
            :from => "any",
            :to   => "map <string, any>"
          )
        end
      end
      Builtins.y2milestone("----------------------------------------")
      Builtins.y2milestone("users plugin started: FingerprintReader")

      Builtins.y2debug("func=%1, config=%2, data=%3", @func, @config, @data)

      if @func == "Summary"
        @ret = UsersPluginFingerprintReader.Summary(@config, {})
      elsif @func == "Name"
        @ret = UsersPluginFingerprintReader.Name(@config, {})
      elsif @func == "Dialog"
        @caption = UsersPluginFingerprintReader.Name(@config, {})
        # help text for fingerprint reader plugin
        @help_text = _(
          "<p>Swipe your finger on the fingerprint reader. Three successful attempts  \nare needed to save the new fingerprint.</p>"
        )
        @username = Ops.get_string(@data, "uid", "")
        if @username == ""
          Builtins.y2error("user name is empty!")
          return :back
        end
        @fingerprint_dir = Ops.add(
          Ops.add(Ops.add(Directory.tmpdir, "/"), @username),
          "/"
        )

        @contents = HBox(
          HSpacing(1.5),
          VBox(
            VSpacing(0.5),
            ReplacePoint(
              Id(:rp),
              # status label
              Label(Id(:label), _("Initializing fingerprint reader..."))
            ),
            ReplacePoint(Id(:rpstatus), VSpacing()),
            ReplacePoint(
              Id(:rpbutton),
              PushButton(Id(:cancel), Label.CancelButton)
            ),
            ReplacePoint(Id(:rpswipes), VSpacing()),
            VSpacing(0.5)
          ),
          HSpacing(1.5)
        )

        Wizard.CreateDialog
        Wizard.SetTitleIcon("yast-fingerprint")

        # dialog caption
        Wizard.SetContentsButtons(
          @caption,
          @contents,
          @help_text,
          Label.CancelButton,
          Label.OKButton
        )

        Wizard.HideAbortButton
        Wizard.HideBackButton
        Wizard.DisableNextButton

        @exit = false
        @ui = nil
        @exit_status = 256
        @swipe_success = 0
        @swipe_failed = 0

        # helper function: return exit status of the fprint proccess;
        # regulary check user input for `cancel button
        def get_exit_status
          ex = @exit_status
          while true
            exited = Convert.to_boolean(SCR.Read(path(".fprint.check_exit")))
            if exited
              ex = Convert.to_integer(SCR.Read(path(".fprint.exit_status")))
              break
            end
            @ui = UI.PollInput
            if @ui == :cancel
              SCR.Execute(path(".fprint.cancel"))
              UI.ReplaceWidget(Id(:rp), Label(_("Canceled")))
              break
            end
            Builtins.sleep(100)
          end
          ex
        end

        # update the status about fingerprint reader
        def status_message(label, message)
          UI.ReplaceWidget(Id(:rp), Label(label))
          UI.ReplaceWidget(Id(:rpstatus), Label(message))

          nil
        end

        # retry acquiring fingerprint
        @retry = true
        while @retry
          @retry = false

          if SCR.Execute(path(".fprint.enroll"), @fingerprint_dir) != true
            @ui = :cancel
            # status message
            UI.ReplaceWidget(
              Id(:rp),
              Label(_("Initialization of fingerprint reader failed."))
            )
          else
            status_message(
              # status label
              _("Device initialized."),
              # status message
              _("Swipe your right index finger.")
            )
          end

          while @ui != :cancel
            @statemap = Convert.to_map(SCR.Read(path(".fprint.state")))
            if @statemap != nil && @statemap != {}
              @state = Ops.get_integer(@statemap, "state", 0)
              Builtins.y2milestone("state: %1", @state)
              case @state
                when 1
                  UI.ReplaceWidget(
                    Id(:rp),
                    # status label
                    Label(_("Storing data..."))
                  )
                  UI.ReplaceWidget(Id(:rpstatus), VSpacing())
                  @exit = true
                when 2
                  status_message(
                    # status label
                    _("Scan failed."),
                    # status message
                    _("Swipe finger again.")
                  )
                when 3
                  @swipe_success = Ops.add(@swipe_success, 1)
                  status_message(
                    # status label
                    _("Enroll stage passed."),
                    # status message
                    _("Swipe finger again.")
                  )
                  UI.ReplaceWidget(
                    Id(:rpswipes),
                    Label(
                      # status message (%1 is number)
                      Builtins.sformat(
                        _("Successful swipes: %1"),
                        @swipe_success
                      )
                    )
                  )
                when 101
                  status_message(
                    # status label
                    _("Scan failed."),
                    # status message
                    _("Swipe was too short, try again.")
                  )
                when 102
                  status_message(
                    # status label
                    _("Scan failed."),
                    # status message
                    _("Center your finger on the sensor and try again.")
                  )
                when 103
                  # status message
                  status_message(
                    # status label
                    _("Scan failed."),
                    # status message
                    _("Remove your finger from the sensor and try again.")
                  )
                when -1
                  # some fatal error
                  status_message(
                    # status label
                    _("Acquiring fingerprint failed."),
                    ""
                  )
                  @exit = true
              end
              if Ops.less_than(@state, -1)
                # some fatal error
                status_message(
                  # status label
                  _("Acquiring fingerprint failed."),
                  ""
                )
                @exit = true
              end
            elsif @statemap == nil
              @exit_status = get_exit_status
              break
            end
            if @exit
              @exit_status = get_exit_status
              break #must be before check for cancel
            end

            @ui = UI.PollInput

            if @ui == :cancel || @ui == :back
              SCR.Execute(path(".fprint.cancel"))
              # status label (canceled after user action)
              UI.ReplaceWidget(Id(:rp), Label(_("Canceled")))
              Builtins.y2milestone("canceled")
              break
            end
            Builtins.sleep(100)
          end
          UI.ReplaceWidget(Id(:rpswipes), VSpacing())
          Builtins.y2milestone("agent exit status: %1", @exit_status)
          if @exit_status == 1
            # status label
            UI.ReplaceWidget(
              Id(:rp),
              Label(_("Fingerprint acquired successfully."))
            )
            UI.ReplaceWidget(Id(:rpstatus), Label(""))
            # new id was already saved
            Wizard.EnableNextButton
            UI.ChangeWidget(Id(:cancel), :Enabled, false)
          else
            # error message, part 1
            @error = _("Could not acquire fingerprint.")
            @details = ""
            # see the error exit values in FPrintAgent.cc
            if @exit_status == 200
              # error message, part 2
              @details = _("Initialization failed.")
            elsif @exit_status == 201
              # error message, part 2
              @details = _("No devices detected.")
            elsif @exit_status == 202
              # error message, part 2
              @details = _("Device could not be opened.")
            elsif @exit_status == 253
              # error message, part 2
              @details = _("USB error occurred.")
            elsif @exit_status == 254
              # error message, part 2
              @details = _("Communication with fingerprint reader failed.")
            end
            status_message(@error, @details) if @exit_status != 256
            if @ui == :cancel
              UI.ChangeWidget(Id(:cancel), :Enabled, false)
            else
              # button label
              UI.ReplaceWidget(
                Id(:rpbutton),
                PushButton(Id(:retry), _("Retry"))
              )
              Wizard.RestoreBackButton
            end
          end
          if @ui != :cancel
            @ret = UI.UserInput
          else
            @ret = :cancel
          end
          if @ret == :next
            # modified data to add to user
            @tmp_data = {
              # this is identifying the subdirectory of common tmp directory:
              "_fingerprint_dir" => @username,
              "plugin_modified"  => 1
            }
            if Ops.get_string(@data, "what", "") == "edit_user"
              Users.EditUser(@tmp_data)
            elsif Ops.get_string(@data, "what", "") == "add_user"
              Users.AddUser(@tmp_data)
            end
          elsif @ret == :retry
            UI.ReplaceWidget(
              Id(:rpbutton),
              PushButton(Id(:cancel), Label.CancelButton)
            )
            Wizard.HideBackButton
            @retry = true
          end
        end
        Wizard.CloseDialog
      end
      Builtins.y2milestone("users plugin finished with %1", @ret)
      Builtins.y2milestone("----------------------------------------")
      deep_copy(@ret)
    end
  end
end

Yast::UsersPluginFingerprintReaderClient.new.main
